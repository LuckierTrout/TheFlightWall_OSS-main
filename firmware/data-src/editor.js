/* FlightWall Layout Editor (LE-1.7) - ES5 only */
(function() {
  'use strict';

  /* ---------- Module globals ---------- */
  var SCALE = 4;                /* CSS pixels per logical pixel */
  var PREVIEW_SCALE = 6;        /* preview panel upscale (denser than editor) */
  var SNAP = 8;                 /* current snap grid: 8 / 16 / 1 */
  var deviceW = 0;              /* logical canvas width (from /api/layout) */
  var deviceH = 0;              /* logical canvas height */
  var tilePixels = 16;          /* tile size in logical pixels */
  var widgets = [];             /* [{type,x,y,w,h,color,content,align,id}] */
  var selectedIndex = -1;       /* -1 = no selection */
  /* Additional selections captured via shift-click. `selectedIndex` remains
     the "primary" (drives the properties panel + keyboard nudge), and
     `selectedIndices` holds every currently-selected widget including the
     primary. Used by the group-align buttons and the multi-outline render
     path; single-widget drag still ignores the extras. */
  var selectedIndices = [];
  var dragState = null;         /* {mode,startX,startY,origX,origY,origW,origH,toastedFloor} */
  var widgetTypeMeta = {};      /* keyed by type string */
  var HANDLE_SIZE = 10;         /* CSS pixels — visible size of the resize handle */
  var HANDLE_HIT_PAD = 6;       /* extra CSS pixels around handle to ease grabbing */
  var dirty = false;            /* true when widgets changed since last save */
  var currentLayoutId = null;   /* string id after first save, or null for new */

  /* Preview state.
     - previewFlights: full snapshot from /api/flights/current. Rotated via
       previewFlightIndex at PREVIEW_CYCLE_MS so the preview samples every
       visible aircraft in turn (mirrors the wall's own cycle behavior when
       multiple flights are in range).
     - previewLiveEnabled: backs the "Live flight data" checkbox.
     - Timers are separated so the cycle advances on schedule even when the
       poll hasn't just landed. */
  var previewFlights = [];
  var previewFlightIndex = 0;
  var previewLiveEnabled = true;
  var previewPollTimer = null;
  var previewCycleTimer = null;
  var PREVIEW_POLL_MS = 5000;
  var PREVIEW_CYCLE_MS = 3000;

  /* Logo bitmap cache keyed by operator ICAO. Values are Uint16Array(1024)
     of little-endian-interpreted RGB565 pixels. The raw file on the device
     is big-endian RGB565 (matches LCD image converter output and the
     firmware's new byte-swap in LogoManager), so we swap each pair on
     ingest here too. Fetches are one-shot per ICAO — logos don't change
     between flights of the same carrier. */
  var logoCache = {};
  var logoFetchInFlight = {};
  var LOGO_PIXEL_COUNT = 32 * 32;
  var LOGO_BYTE_COUNT = LOGO_PIXEL_COUNT * 2;

  function fetchLogoBitmap(icao) {
    if (!icao) return;
    if (logoCache[icao] || logoFetchInFlight[icao]) return;
    logoFetchInFlight[icao] = true;
    // XHR because we need arraybuffer response type, which FW.get doesn't
    // expose. Keep it scoped and defensive — a failed logo just stays as a
    // placeholder; no user-facing error.
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/logos/' + encodeURIComponent(icao) + '.bin', true);
    xhr.responseType = 'arraybuffer';
    xhr.onload = function () {
      logoFetchInFlight[icao] = false;
      if (xhr.status !== 200 || !xhr.response) return;
      var buf = xhr.response;
      if (buf.byteLength !== LOGO_BYTE_COUNT) return;
      var view = new Uint8Array(buf);
      var pixels = new Uint16Array(LOGO_PIXEL_COUNT);
      for (var i = 0; i < LOGO_PIXEL_COUNT; i++) {
        var hi = view[i * 2];       // big-endian on disk
        var lo = view[i * 2 + 1];
        pixels[i] = (hi << 8) | lo;
      }
      logoCache[icao] = pixels;
      renderPreview();
    };
    xhr.onerror = function () { logoFetchInFlight[icao] = false; };
    xhr.send();
  }

  /* Synthetic flight used when live data is off or /api/flights/current returns
     an empty list. Mirrors the FlightInfo schema so widget resolvers work. */
  var SAMPLE_FLIGHT = {
    ident: 'UAL123', ident_icao: 'UAL123', ident_iata: 'UA123',
    operator_code: 'UAL', operator_icao: 'UAL', operator_iata: 'UA',
    origin_icao: 'KLAX', destination_icao: 'KJFK',
    aircraft_code: 'B738', airline_display_name_full: 'United Airlines',
    aircraft_display_name_short: '737-800',
    altitude_kft: 34.0, speed_mph: 518, track_deg: 72,
    vertical_rate_fps: 0.0, distance_km: 42.5, bearing_deg: 110
  };

  /* ---------- Utilities ---------- */
  function snapTo(val, grid) {
    return Math.round(val / grid) * grid;
  }

  function clamp(val, lo, hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
  }

  function getCanvasPos(e) {
    var canvas = document.getElementById('editor-canvas');
    var rect = canvas.getBoundingClientRect();
    return { x: e.clientX - rect.left, y: e.clientY - rect.top };
  }

  function setStatus(msg) {
    var el = document.getElementById('editor-status');
    if (el) el.textContent = msg || '';
  }

  /* ---------- Render ---------- */
  function render() {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    /* 1. Clear */
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    /* 2. Tile grid */
    drawGrid(ctx);
    /* 3. Widgets. Pass an explicit "is primary" flag (drives the blue
          resize handle + grip pattern) and a "is in multi-select" flag
          (thinner magenta outline so the group is visually distinct from
          the single primary selection). */
    for (var i = 0; i < widgets.length; i++) {
      var isPrimary = (i === selectedIndex);
      var isInGroup = !isPrimary && selectedIndices.indexOf(i) >= 0;
      drawWidget(ctx, widgets[i], isPrimary, isInGroup);
    }
    /* 3b. Snap guides (drawn on top of widgets, below the preview). */
    drawSnapGuides(ctx);
    /* 4. Mirror to the pixel-accurate preview panel. Guarded on FWPreview
          being loaded so the editor still works on pages that haven't
          pulled in preview.js for some reason. */
    renderPreview();
  }

  /* Resolve which flight the preview should paint with right now. When live
     data is off we always use the synthetic sample; when on, we pick the
     current cycle index (if flights are available) and fall back to the
     sample when the list is empty. */
  function activePreviewFlight() {
    if (!previewLiveEnabled) return SAMPLE_FLIGHT;
    if (!previewFlights.length) return SAMPLE_FLIGHT;
    var idx = previewFlightIndex % previewFlights.length;
    return previewFlights[idx] || SAMPLE_FLIGHT;
  }

  /* Paint the preview canvas using the current widget list + cached flight
     data. Called after every render() so edits show up as you drag.
     Kicks off a one-shot logo fetch for the active flight's operator ICAO
     so subsequent renders can include the real bitmap instead of the
     dashed placeholder. */
  function renderPreview() {
    if (!window.FWPreview) return;
    var canvas = document.getElementById('preview-canvas');
    if (!canvas || !deviceW || !deviceH) return;
    var flight = activePreviewFlight();
    if (flight && flight.operator_icao) fetchLogoBitmap(flight.operator_icao);
    window.FWPreview.render({
      canvas: canvas,
      matrixW: deviceW,
      matrixH: deviceH,
      scale: PREVIEW_SCALE,
      widgets: widgets,
      flight: flight,
      logoCache: logoCache,
      background: '#000'
    });
    updatePreviewHint();
  }

  /* Annotates the hint line with useful context: which flight is active,
     cycling position, and whether the flight is General Aviation (no
     airline/operator) — that's the most common source of "why is the
     Airline widget showing '--'?" confusion. */
  function isGeneralAviation(f) {
    if (!f) return false;
    var noAirline = !f.airline_display_name_full && !f.operator_icao &&
                    !f.operator_iata && !f.operator_code;
    return noAirline;
  }

  function updatePreviewHint() {
    var hint = document.getElementById('preview-panel-hint');
    if (!hint) return;
    if (!previewLiveEnabled) {
      hint.textContent = 'Preview uses sample flight data (toggle off).';
      return;
    }
    if (!previewFlights.length) {
      hint.textContent = 'No flights in range — using sample data.';
      return;
    }
    var f = activePreviewFlight();
    var ident = f.ident || f.ident_icao || 'flight';
    var total = previewFlights.length;
    var current = (previewFlightIndex % total) + 1;
    var gaNote = isGeneralAviation(f) ? ' — GA, no airline data' : '';
    hint.textContent = 'Showing ' + current + ' of ' + total + ': ' + ident + gaNote;
  }

  function drawGrid(ctx) {
    var canvas = ctx.canvas;
    var step = tilePixels * SCALE;
    if (step <= 0) return;
    ctx.strokeStyle = 'rgba(255,255,255,0.1)';
    ctx.lineWidth = 1;
    var x;
    for (x = step; x < canvas.width; x += step) {
      ctx.beginPath();
      ctx.moveTo(x + 0.5, 0);
      ctx.lineTo(x + 0.5, canvas.height);
      ctx.stroke();
    }
    var y;
    for (y = step; y < canvas.height; y += step) {
      ctx.beginPath();
      ctx.moveTo(0, y + 0.5);
      ctx.lineTo(canvas.width, y + 0.5);
      ctx.stroke();
    }
  }

  /* Draw the per-axis snap guides captured during the current drag. The
     guide list on dragState is populated by findSnapTargets() — each entry
     is `{ axis: 'x' | 'y', value: <logical pixel> }`. Magenta lines stand
     out against the #111 background and don't collide with the #FFF
     selection outline or blue handle. */
  function drawSnapGuides(ctx) {
    if (!dragState || !dragState.guides || !dragState.guides.length) return;
    ctx.save();
    ctx.strokeStyle = '#ff5cff';
    ctx.setLineDash([4, 4]);
    ctx.lineWidth = 1;
    for (var i = 0; i < dragState.guides.length; i++) {
      var g = dragState.guides[i];
      ctx.beginPath();
      if (g.axis === 'x') {
        var x = g.value * SCALE + 0.5;
        ctx.moveTo(x, 0);
        ctx.lineTo(x, ctx.canvas.height);
      } else {
        var y = g.value * SCALE + 0.5;
        ctx.moveTo(0, y);
        ctx.lineTo(ctx.canvas.width, y);
      }
      ctx.stroke();
    }
    ctx.restore();
  }

  function drawWidget(ctx, w, isSelected, isInGroup) {
    var x = w.x * SCALE;
    var y = w.y * SCALE;
    var cw = w.w * SCALE;
    var ch = w.h * SCALE;
    /* Body fill at 80% opacity */
    ctx.globalAlpha = 0.8;
    ctx.fillStyle = w.color || '#444';
    ctx.fillRect(x, y, cw, ch);
    ctx.globalAlpha = 1.0;
    /* Type label */
    ctx.fillStyle = '#FFF';
    ctx.font = (SCALE * 6) + 'px monospace';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(w.type, x + cw / 2, y + ch / 2);
    /* Multi-select ring — dashed magenta outline on non-primary members
       so the user can tell them apart from the primary (white outline,
       blue handle). Draw *before* the primary highlight so they stack
       correctly if somehow both flags were true. */
    if (isInGroup) {
      ctx.save();
      ctx.strokeStyle = '#ff5cff';
      ctx.setLineDash([3, 3]);
      ctx.lineWidth = 2;
      ctx.strokeRect(x + 1, y + 1, cw - 2, ch - 2);
      ctx.restore();
    }
    /* Selection outline + resize handle. The handle is deliberately bigger
       than the default square so it's findable on a trackpad; its hit zone
       in hitTest() extends another HANDLE_HIT_PAD around it for forgiving
       grabs. A grip pattern (two stacked triangles) makes the affordance
       visually unambiguous. */
    if (isSelected) {
      ctx.strokeStyle = '#FFF';
      ctx.lineWidth = 2;
      ctx.strokeRect(x + 1, y + 1, cw - 2, ch - 2);

      var hx = x + cw - HANDLE_SIZE;
      var hy = y + ch - HANDLE_SIZE;
      ctx.fillStyle = '#58a6ff';
      ctx.fillRect(hx, hy, HANDLE_SIZE, HANDLE_SIZE);
      ctx.strokeStyle = '#FFF';
      ctx.lineWidth = 1;
      ctx.strokeRect(hx + 0.5, hy + 0.5, HANDLE_SIZE - 1, HANDLE_SIZE - 1);
      // Diagonal grip lines so the handle reads as "draggable corner".
      ctx.beginPath();
      ctx.moveTo(hx + HANDLE_SIZE - 2, hy + 3);
      ctx.lineTo(hx + 3, hy + HANDLE_SIZE - 2);
      ctx.moveTo(hx + HANDLE_SIZE - 2, hy + 6);
      ctx.lineTo(hx + 6, hy + HANDLE_SIZE - 2);
      ctx.stroke();
    }
  }

  /* ---------- Canvas init ---------- */
  function initCanvas(matrixW, matrixH, tilePx) {
    deviceW = matrixW;
    deviceH = matrixH;
    tilePixels = tilePx || 16;
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    canvas.width = matrixW * SCALE;
    canvas.height = matrixH * SCALE;
    canvas.style.width = (matrixW * SCALE) + 'px';
    canvas.style.height = (matrixH * SCALE) + 'px';
    render();
  }

  /* ---------- Hit testing ---------- */
  function hitTest(cx, cy) {
    /* 1. Resize handle of selected widget. The handle is drawn HANDLE_SIZE
       CSS px wide in the bottom-right corner; we extend the hit zone by
       HANDLE_HIT_PAD on every side (including *outside* the widget body)
       so users can grab it with a trackpad without pixel-perfect aim. */
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
      var sw = widgets[selectedIndex];
      var handleX0 = sw.x * SCALE + sw.w * SCALE - HANDLE_SIZE;
      var handleY0 = sw.y * SCALE + sw.h * SCALE - HANDLE_SIZE;
      var hitX0 = handleX0 - HANDLE_HIT_PAD;
      var hitY0 = handleY0 - HANDLE_HIT_PAD;
      var hitX1 = handleX0 + HANDLE_SIZE + HANDLE_HIT_PAD;
      var hitY1 = handleY0 + HANDLE_SIZE + HANDLE_HIT_PAD;
      if (cx >= hitX0 && cx <= hitX1 && cy >= hitY0 && cy <= hitY1) {
        return { index: selectedIndex, mode: 'resize' };
      }
    }
    /* 2. Widget body (topmost first) */
    for (var i = widgets.length - 1; i >= 0; i--) {
      var w = widgets[i];
      var x0 = w.x * SCALE;
      var y0 = w.y * SCALE;
      var x1 = (w.x + w.w) * SCALE;
      var y1 = (w.y + w.h) * SCALE;
      if (cx >= x0 && cx <= x1 && cy >= y0 && cy <= y1) {
        return { index: i, mode: 'move' };
      }
    }
    return null;
  }

  /* Update the canvas cursor to match what the user is about to do. Runs on
     every pointer move (not just drags) so the cursor signals affordances:
     'nwse-resize' over the handle, 'move' over a widget body, default
     otherwise. Skipped while a drag is in progress — the browser already
     retains the starting cursor during pointer capture. */
  function updateHoverCursor(cx, cy) {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    if (dragState !== null) return;
    var hit = hitTest(cx, cy);
    if (hit && hit.mode === 'resize') canvas.style.cursor = 'nwse-resize';
    else if (hit && hit.mode === 'move') canvas.style.cursor = 'move';
    else canvas.style.cursor = 'default';
  }

  /* Selection helpers. `selectedIndex` is the primary (used for drag +
     props panel) and `selectedIndices` is the full multi-select group used
     by group-align ops. Keeping them consistent — the primary is always
     included in selectedIndices when selection is non-empty. */
  function setSingleSelection(idx) {
    selectedIndex = idx;
    selectedIndices = (idx >= 0) ? [idx] : [];
  }
  function toggleSelectionAt(idx) {
    var pos = selectedIndices.indexOf(idx);
    if (pos >= 0) {
      selectedIndices.splice(pos, 1);
      if (selectedIndex === idx) {
        selectedIndex = selectedIndices.length ? selectedIndices[0] : -1;
      }
    } else {
      selectedIndices.push(idx);
      selectedIndex = idx;  // newest shift-click becomes primary
    }
  }

  /* ---------- Pointer events ---------- */
  function onPointerDown(e) {
    var canvas = document.getElementById('editor-canvas');
    var p = getCanvasPos(e);
    var hit = hitTest(p.x, p.y);
    if (hit) {
      if (e.shiftKey && hit.mode === 'move') {
        // Shift+click extends the group. Never starts a drag, to keep the
        // single-widget drag model simple — the click is purely selection.
        toggleSelectionAt(hit.index);
        dragState = null;
      } else {
        setSingleSelection(hit.index);
        var w = widgets[selectedIndex];
        dragState = {
          mode: hit.mode,
          startX: e.clientX,
          startY: e.clientY,
          origX: w.x,
          origY: w.y,
          origW: w.w,
          origH: w.h,
          toastedFloor: false
        };
        try { canvas.setPointerCapture(e.pointerId); } catch (err) { /* ignore */ }
      }
      e.preventDefault();
    } else {
      setSingleSelection(-1);
      dragState = null;
    }
    render();
    showPropsPanel(selectedIndex >= 0 ? widgets[selectedIndex] : null);
  }

  /* Scan all OTHER widgets for alignment candidates against a candidate
     position (nx, ny) of the moving widget. A match within SNAP_THRESHOLD
     pixels on an axis snaps to the neighbor's edge/center and records a
     guide line so the drag visualization can render it. Checks six
     alignment kinds per axis: left-left, right-right, center-center,
     right-left (edge-to-edge), left-right, and (for Y) their vertical
     analogues. */
  var SNAP_THRESHOLD = 3; // logical pixels
  function findSnapTargets(moving, nx, ny) {
    var out = { x: nx, y: ny, guides: [] };
    var mL = nx, mR = nx + moving.w, mCX = nx + (moving.w / 2 | 0);
    var mT = ny, mB = ny + moving.h, mCY = ny + (moving.h / 2 | 0);
    var bestDX = SNAP_THRESHOLD + 1, bestDY = SNAP_THRESHOLD + 1;
    var snappedX = nx, snappedY = ny;
    var guideX = null, guideY = null;
    for (var i = 0; i < widgets.length; i++) {
      if (i === selectedIndex) continue;
      var o = widgets[i];
      var oL = o.x, oR = o.x + o.w, oCX = o.x + (o.w / 2 | 0);
      var oT = o.y, oB = o.y + o.h, oCY = o.y + (o.h / 2 | 0);
      // X-axis candidates: [moving-edge-to-match, other-target, resulting-x]
      var candidatesX = [
        [mL,  oL,  oL],              // align left edges
        [mR,  oR,  oR - moving.w],   // align right edges
        [mCX, oCX, oCX - (moving.w / 2 | 0)], // center X match
        [mL,  oR,  oR],              // left-to-right (edge touch)
        [mR,  oL,  oL - moving.w]    // right-to-left
      ];
      for (var cx = 0; cx < candidatesX.length; cx++) {
        var c = candidatesX[cx];
        var d = Math.abs(c[0] - c[1]);
        if (d < bestDX) {
          bestDX = d;
          snappedX = c[2];
          guideX = c[1];
        }
      }
      var candidatesY = [
        [mT,  oT,  oT],
        [mB,  oB,  oB - moving.h],
        [mCY, oCY, oCY - (moving.h / 2 | 0)],
        [mT,  oB,  oB],
        [mB,  oT,  oT - moving.h]
      ];
      for (var cy = 0; cy < candidatesY.length; cy++) {
        var cc = candidatesY[cy];
        var dd = Math.abs(cc[0] - cc[1]);
        if (dd < bestDY) {
          bestDY = dd;
          snappedY = cc[2];
          guideY = cc[1];
        }
      }
    }
    if (bestDX <= SNAP_THRESHOLD) {
      out.x = snappedX;
      out.guides.push({ axis: 'x', value: guideX });
    }
    if (bestDY <= SNAP_THRESHOLD) {
      out.y = snappedY;
      out.guides.push({ axis: 'y', value: guideY });
    }
    return out;
  }

  function onPointerMove(e) {
    if (dragState === null) {
      // Not dragging — only update the hover cursor so users get a visual
      // affordance before they click (resize vs move vs empty canvas).
      var hoverPos = getCanvasPos(e);
      updateHoverCursor(hoverPos.x, hoverPos.y);
      return;
    }
    var w = widgets[selectedIndex];
    if (!w) { dragState = null; return; }
    var dxCss = e.clientX - dragState.startX;
    var dyCss = e.clientY - dragState.startY;
    var dx = dxCss / SCALE;
    var dy = dyCss / SCALE;

    if (dragState.mode === 'move') {
      var nx = snapTo(dragState.origX + dx, SNAP);
      var ny = snapTo(dragState.origY + dy, SNAP);
      // Layer alignment-to-neighbor snapping on top of the grid snap. This
      // only applies when a peer edge/center is within SNAP_THRESHOLD; the
      // grid result stands when no peer is close. guides[] drives the
      // magenta lines drawn on top of the canvas during drag.
      var snap = findSnapTargets(w, nx, ny);
      dragState.guides = snap.guides;
      w.x = clamp(snap.x, 0, deviceW - w.w);
      w.y = clamp(snap.y, 0, deviceH - w.h);
    } else if (dragState.mode === 'resize') {
      var meta = widgetTypeMeta[w.type] || { minW: 6, minH: 6 };
      var rawW = snapTo(dragState.origW + dx, SNAP);
      var rawH = snapTo(dragState.origH + dy, SNAP);
      var maxW = deviceW - w.x;
      var maxH = deviceH - w.y;
      var newW = clamp(rawW, meta.minW, maxW);
      var newH = clamp(rawH, meta.minH, maxH);
      if ((rawW < meta.minW || rawH < meta.minH) && !dragState.toastedFloor) {
        if (typeof FW !== 'undefined' && FW.showToast) {
          FW.showToast(w.type + ' minimum: ' + meta.minW + 'w \u00d7 ' + meta.minH + 'h px', 'warning');
        }
        dragState.toastedFloor = true;
      }
      w.w = newW;
      w.h = newH;
    }
    render();
    setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
    /* Update position/size readouts in property panel during drag */
    var posReadout  = document.getElementById('prop-pos-readout');
    var sizeReadout = document.getElementById('prop-size-readout');
    if (posReadout)  posReadout.textContent  = w.x + ', ' + w.y;
    if (sizeReadout) sizeReadout.textContent = w.w + ' \u00d7 ' + w.h;
  }

  function onPointerUp(e) {
    if (dragState === null) return;
    /* Mark dirty on completed drag/resize before clearing state */
    dirty = true;
    /* Release pointer capture before clearing dragState (spec-correct order) */
    var canvas = document.getElementById('editor-canvas');
    if (canvas && e && e.pointerId != null) {
      try { canvas.releasePointerCapture(e.pointerId); } catch (err) { /* ignore */ }
    }
    dragState = null;
    // Force a repaint so the snap guides drawn during the drag are cleared
    // now that dragState is null.
    render();
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
      var w = widgets[selectedIndex];
      setStatus(w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
    }
  }

  /* ---------- Toolbox ---------- */
  function buildWidgetTypeMeta(entry) {
    var meta = {
      defaultW: 32, defaultH: 8, defaultColor: '#FFFFFF',
      defaultContent: '', defaultAlign: 'left',
      defaultFont: 'default',   /* widget font id (matches firmware enum) */
      defaultTextSize: 1,       /* integer GFX setTextSize factor */
      defaultTextTransform: 'none', /* widget case transform; matches firmware enum */
      minW: 6, minH: 6,
      contentKind: 'string',   /* 'string' | 'select' — drives the props panel */
      contentOptions: null,    /* [ {value, label} ] when contentKind==='select' */
      contentLabel: 'Value',   /* props-panel label override (LE-1.10) */
      fontOptions: null,       /* [{id,label}] when widget supports font choice */
      sizeOptions: null,       /* [{id,label}] when widget supports size choice */
      transformOptions: null   /* [{id,label}] when widget supports case toggle */
    };
    if (entry.fields) {
      for (var i = 0; i < entry.fields.length; i++) {
        var f = entry.fields[i];
        if (f.key === 'w')       meta.defaultW = (f['default'] != null) ? f['default'] : 32;
        if (f.key === 'h')       meta.defaultH = (f['default'] != null) ? f['default'] : 8;
        if (f.key === 'color')   meta.defaultColor = f['default'] || '#FFFFFF';
        if (f.key === 'content') {
          meta.defaultContent = (f['default'] != null) ? f['default'] : '';
          /* LE-1.8: content field may be a select (flight_field, metric, clock).
             Capture its kind/options so showPropsPanel can render a dropdown
             sourced from the API — no hard-coded option lists in the UI. */
          meta.contentKind = (f.kind === 'select') ? 'select' : 'string';
          if (f.kind === 'select' && f.options && f.options.length) {
            meta.contentOptions = [];
            for (var oi = 0; oi < f.options.length; oi++) {
              meta.contentOptions.push({
                value: f.options[oi],
                label: f.options[oi]
              });
            }
          }
        }
        if (f.key === 'align')   meta.defaultAlign = f['default'] || 'left';
        if (f.key === 'font')    meta.defaultFont = f['default'] || 'default';
        if (f.key === 'text_size') {
          var ts = parseInt(f['default'] || '1', 10);
          meta.defaultTextSize = (isNaN(ts) || ts < 1) ? 1 : ts;
        }
        if (f.key === 'text_transform') {
          meta.defaultTextTransform = f['default'] || 'none';
        }
      }
    }
    /* Font / size option lists come from sibling keys so they carry
       human-readable labels without cluttering the `fields` schema.
       Only widget types that opt in (text for now) will populate these. */
    if (entry.font_options && entry.font_options.length) {
      meta.fontOptions = [];
      for (var fOptI = 0; fOptI < entry.font_options.length; fOptI++) {
        var fopt = entry.font_options[fOptI];
        if (fopt && fopt.id) {
          meta.fontOptions.push({ value: fopt.id, label: fopt.label || fopt.id });
        }
      }
    }
    if (entry.size_options && entry.size_options.length) {
      meta.sizeOptions = [];
      for (var sOptI = 0; sOptI < entry.size_options.length; sOptI++) {
        var sopt = entry.size_options[sOptI];
        if (sopt && sopt.id) {
          meta.sizeOptions.push({ value: sopt.id, label: sopt.label || sopt.id });
        }
      }
    }
    if (entry.transform_options && entry.transform_options.length) {
      meta.transformOptions = [];
      for (var tOptI = 0; tOptI < entry.transform_options.length; tOptI++) {
        var topt = entry.transform_options[tOptI];
        if (topt && topt.id) {
          meta.transformOptions.push({ value: topt.id, label: topt.label || topt.id });
        }
      }
    }
    /* LE-1.10: prefer entry.field_options [{id,label,unit?}] when present —
       it carries human-readable labels for the field-picker dropdown.
       Overrides the raw string options from the content schema. */
    if (entry.field_options && entry.field_options.length) {
      meta.contentOptions = [];
      for (var fi = 0; fi < entry.field_options.length; fi++) {
        var fo = entry.field_options[fi];
        if (!fo || !fo.id) {
          continue;
        }
        if (!fo || !fo.id) continue;
        var labelText = fo.label || fo.id;
        // Guard against duplication: the firmware's metric catalog already
        // embeds the unit in the label (e.g. "Speed (kts)"), so appending
        // "(kts)" again produces "Speed (kts) (kts)". Only append when the
        // label doesn't already end with the exact "(unit)" suffix.
        if (fo.unit) {
          var suffix = '(' + fo.unit + ')';
          if (labelText.substr(labelText.length - suffix.length) !== suffix) {
            labelText = labelText + ' ' + suffix;
          }
        }
        meta.contentOptions.push({ value: fo.id, label: labelText });
      }
      meta.contentKind = 'select';
      meta.contentLabel = 'Field';
      if (!meta.defaultContent && meta.contentOptions.length) {
        meta.defaultContent = meta.contentOptions[0].value;
      }
    }
    /* Min floors: firmware enforces w<8||h<8 early-return; use h.default as floor */
    meta.minH = meta.defaultH;
    meta.minW = 6;
    widgetTypeMeta[entry.type] = meta;
  }

  function isDisabledEntry(entry) {
    return !!(entry.note && entry.note.indexOf('not yet implemented') !== -1);
  }

  function initToolbox(types) {
    var box = document.getElementById('editor-toolbox');
    if (!box) return;
    box.innerHTML = '';
    for (var i = 0; i < types.length; i++) {
      buildWidgetTypeMeta(types[i]);
      var entry = types[i];
      var item = document.createElement('div');
      item.className = 'toolbox-item';
      if (isDisabledEntry(entry)) item.className += ' disabled';
      item.setAttribute('data-type', entry.type);
      item.textContent = entry.label || entry.type;
      box.appendChild(item);
    }
    box.addEventListener('click', onToolboxClick);
  }

  function onToolboxClick(e) {
    var t = e.target;
    if (!t || !t.classList || !t.classList.contains('toolbox-item')) return;
    if (t.classList.contains('disabled')) return;
    var type = t.getAttribute('data-type');
    if (type) addWidget(type);
  }

  /* Inherent padding applied when auto-placing a newly added widget, so
     fresh designs don't start out with everything crammed against (0,0).
     Users can still drag widgets edge-to-edge by hand; this just gives a
     breathing-room default. */
  var EDGE_MARGIN = 2;
  var WIDGET_GAP = 2;

  /* Pick a default (x, y) for a new widget that avoids overlapping any
     existing widget. Strategy: stack vertically below the lowest bottom
     edge currently in use, with a WIDGET_GAP buffer. If there's no room
     left in that column, fall back to the edge margin and let the user
     drag. Snaps the result to the active grid so positions stay tidy. */
  function pickNextPlacement(widgetW, widgetH) {
    var x = snapTo(EDGE_MARGIN, SNAP);
    var y = snapTo(EDGE_MARGIN, SNAP);
    if (!widgets.length) return { x: x, y: y };

    var bottom = 0;
    for (var i = 0; i < widgets.length; i++) {
      var wi = widgets[i];
      var wBottom = (wi.y | 0) + (wi.h | 0);
      if (wBottom > bottom) bottom = wBottom;
    }
    var desiredY = bottom + WIDGET_GAP;
    var snappedY = snapTo(desiredY, SNAP);
    // snapTo rounds (can round down) — bump up a grid step if the snap
    // pulled us back into the previous widget's footprint.
    if (snappedY < desiredY) snappedY += SNAP;

    // If the new widget wouldn't fit vertically, give up on cascading and
    // drop it at the top-left so the user sees it and can reposition.
    if (snappedY + widgetH + EDGE_MARGIN > deviceH) {
      return { x: x, y: y };
    }
    return { x: x, y: snappedY };
  }

  function addWidget(type) {
    var meta = widgetTypeMeta[type];
    if (!meta) return;
    var placement = pickNextPlacement(meta.defaultW, meta.defaultH);
    var w = {
      type: type,
      x: placement.x,
      y: placement.y,
      w: meta.defaultW,
      h: meta.defaultH,
      color: meta.defaultColor || '#FFFFFF',
      content: meta.defaultContent || '',
      align: meta.defaultAlign || 'left',
      font: meta.defaultFont || 'default',
      text_size: meta.defaultTextSize || 1,
      text_transform: meta.defaultTextTransform || 'none',
      id: 'w' + Date.now()
    };
    /* Clamp dimensions to canvas bounds (defensive: guards future widget types) */
    if (deviceW > 0 && w.w > deviceW) w.w = Math.max(meta.minW || 6, deviceW);
    if (deviceH > 0 && w.h > deviceH) w.h = Math.max(meta.minH || 6, deviceH);
    /* Clamp position so widget stays within canvas */
    if (w.x + w.w > deviceW) w.x = Math.max(0, deviceW - w.w);
    if (w.y + w.h > deviceH) w.y = Math.max(0, deviceH - w.h);
    widgets.push(w);
    selectedIndex = widgets.length - 1;
    dirty = true;
    render();
    setStatus('Added ' + type);
    showPropsPanel(widgets[selectedIndex]);
  }

  /* ---------- Snap control ---------- */
  function updateSnapButtons() {
    var btns = document.querySelectorAll('#snap-control .snap-btn');
    for (var i = 0; i < btns.length; i++) {
      var b = btns[i];
      var v = parseInt(b.getAttribute('data-snap'), 10);
      if (v === SNAP) {
        b.className = 'snap-btn active';
      } else {
        b.className = 'snap-btn';
      }
    }
  }

  function initSnap() {
    var stored = null;
    try { stored = window.localStorage.getItem('fw_editor_snap'); } catch (err) { /* ignore */ }
    var v = parseInt(stored, 10);
    if (v === 1 || v === 8 || v === 16) {
      SNAP = v;
    } else {
      SNAP = 8;
    }
    var ctrl = document.getElementById('snap-control');
    if (ctrl) {
      ctrl.addEventListener('click', function(e) {
        var t = e.target;
        if (!t || !t.classList || !t.classList.contains('snap-btn')) return;
        var snapVal = parseInt(t.getAttribute('data-snap'), 10);
        if (snapVal === 1 || snapVal === 8 || snapVal === 16) {
          SNAP = snapVal;
          try { window.localStorage.setItem('fw_editor_snap', String(SNAP)); } catch (err) { /* ignore */ }
          updateSnapButtons();
          render();
        }
      });
    }
    updateSnapButtons();
  }

  /* ---------- Property panel ---------- */
  function updateAlignButtons(align) {
    var btns = document.querySelectorAll('#align-control .align-btn');
    for (var i = 0; i < btns.length; i++) {
      var b = btns[i];
      if (b.getAttribute('data-align') === align) {
        b.className = 'align-btn active';
      } else {
        b.className = 'align-btn';
      }
    }
  }

  function showPropsPanel(w) {
    var placeholder = document.getElementById('props-placeholder');
    var form = document.getElementById('props-form');
    if (!w) {
      if (placeholder) placeholder.style.display = '';
      if (form) form.style.display = 'none';
      return;
    }
    if (placeholder) placeholder.style.display = 'none';
    if (form) form.style.display = '';

    /* content field — text input or select, driven by widgetTypeMeta.
       LE-1.8: flight_field and metric use select dropdowns with options
       coming from the /api/widgets/types response (no hard-coded lists). */
    var contentInput  = document.getElementById('prop-content');
    var contentSelect = document.getElementById('prop-content-select');
    var contentField  = document.getElementById('props-field-content');
    if (contentField) {
      var meta = widgetTypeMeta[w.type] || null;
      var useSelect = !!(meta && meta.contentKind === 'select' &&
                         meta.contentOptions && meta.contentOptions.length);
      if (contentInput)  contentInput.style.display  = useSelect ? 'none' : '';
      if (contentSelect) contentSelect.style.display = useSelect ? '' : 'none';
      if (useSelect && contentSelect) {
        /* Rebuild options when the widget type changes. Each call clears
           any previously-injected options so switching widgets never leaks
           stale entries (ES5 — no Array.from/forEach). Options are now
           {value,label} pairs (LE-1.10 field_options) so the dropdown shows
           a human label while persisting the catalog id. */
        while (contentSelect.firstChild) {
          contentSelect.removeChild(contentSelect.firstChild);
        }
        for (var ci = 0; ci < meta.contentOptions.length; ci++) {
          var optEntry = meta.contentOptions[ci];
          var opt = document.createElement('option');
          opt.value = optEntry.value;
          opt.textContent = optEntry.label;
          contentSelect.appendChild(opt);
        }
        var desired = w.content || meta.defaultContent ||
                       meta.contentOptions[0].value;
        /* If the widget's current content value is not in the option set
           (e.g. imported from an older layout with pre-LE-1.10 keys), fall
           back to the default so the <select> always has a valid selection. */
        var found = false;
        for (var mi = 0; mi < meta.contentOptions.length; mi++) {
          if (meta.contentOptions[mi].value === desired) { found = true; break; }
        }
        var coerced = found ? desired : meta.contentOptions[0].value;
        contentSelect.value = coerced;
        /* Sync in-memory widget if the content value was coerced to a valid
           option (stale/invalid keys from older layouts). Without this, save
           would persist the invalid key even though the UI shows the coerced
           value (programmatic .value = x does not fire 'change'). Also mark
           dirty so the operator is prompted to save the repaired layout. */
        if (!found && selectedIndex >= 0 && widgets[selectedIndex]) {
          widgets[selectedIndex].content = coerced;
          dirty = true;
        }
      } else if (contentInput) {
        contentInput.value = w.content || '';
      }
      /* LE-1.10: override the content-field label when the widget exposes a
         field catalog — "Field" reads better than "Value" for the picker. */
      var contentLabelEl = contentField.querySelector('label');
      if (contentLabelEl && meta && meta.contentLabel) {
        contentLabelEl.textContent = meta.contentLabel;
      }
    }

    /* color */
    var colorInput = document.getElementById('prop-color');
    if (colorInput) colorInput.value = w.color || '#ffffff';

    /* align — only shown for text widget */
    var alignField = document.getElementById('props-field-align');
    if (alignField) {
      alignField.style.display = (w.type === 'text') ? '' : 'none';
      updateAlignButtons(w.align || 'left');
    }

    /* Font + text size — shown only when the widget's /api/widgets/types
       schema exposed font_options / size_options (text widget for now).
       The selects are rebuilt from meta every time we show the panel so
       type switches never leak stale options. */
    var fontField = document.getElementById('props-field-font');
    var fontSelect = document.getElementById('prop-font');
    if (fontField && fontSelect) {
      var meta2 = widgetTypeMeta[w.type] || null;
      var showFont = !!(meta2 && meta2.fontOptions && meta2.fontOptions.length);
      fontField.style.display = showFont ? '' : 'none';
      if (showFont) {
        while (fontSelect.firstChild) fontSelect.removeChild(fontSelect.firstChild);
        for (var fi = 0; fi < meta2.fontOptions.length; fi++) {
          var fopt = meta2.fontOptions[fi];
          var fOpt = document.createElement('option');
          fOpt.value = fopt.value; fOpt.textContent = fopt.label;
          fontSelect.appendChild(fOpt);
        }
        fontSelect.value = w.font || meta2.defaultFont || 'default';
      }
    }
    var sizeField = document.getElementById('props-field-text-size');
    var sizeSelect = document.getElementById('prop-text-size');
    if (sizeField && sizeSelect) {
      var meta3 = widgetTypeMeta[w.type] || null;
      var showSize = !!(meta3 && meta3.sizeOptions && meta3.sizeOptions.length);
      sizeField.style.display = showSize ? '' : 'none';
      if (showSize) {
        while (sizeSelect.firstChild) sizeSelect.removeChild(sizeSelect.firstChild);
        for (var si = 0; si < meta3.sizeOptions.length; si++) {
          var sopt = meta3.sizeOptions[si];
          var sOpt = document.createElement('option');
          sOpt.value = sopt.value; sOpt.textContent = sopt.label;
          sizeSelect.appendChild(sOpt);
        }
        sizeSelect.value = String(w.text_size || meta3.defaultTextSize || 1);
      }
    }
    var transformField = document.getElementById('props-field-text-transform');
    var transformSelect = document.getElementById('prop-text-transform');
    if (transformField && transformSelect) {
      var meta4 = widgetTypeMeta[w.type] || null;
      var showTransform = !!(meta4 && meta4.transformOptions && meta4.transformOptions.length);
      transformField.style.display = showTransform ? '' : 'none';
      if (showTransform) {
        while (transformSelect.firstChild) transformSelect.removeChild(transformSelect.firstChild);
        for (var ti = 0; ti < meta4.transformOptions.length; ti++) {
          var topt = meta4.transformOptions[ti];
          var tOpt = document.createElement('option');
          tOpt.value = topt.value; tOpt.textContent = topt.label;
          transformSelect.appendChild(tOpt);
        }
        transformSelect.value = w.text_transform || meta4.defaultTextTransform || 'none';
      }
    }

    /* position + size readouts */
    var posReadout  = document.getElementById('prop-pos-readout');
    var sizeReadout = document.getElementById('prop-size-readout');
    if (posReadout)  posReadout.textContent  = w.x + ', ' + w.y;
    if (sizeReadout) sizeReadout.textContent = w.w + ' \u00d7 ' + w.h;
  }

  function bindPropsPanelEvents() {
    var contentInput  = document.getElementById('prop-content');
    var contentSelect = document.getElementById('prop-content-select');
    var colorInput    = document.getElementById('prop-color');
    var alignCtrl     = document.getElementById('align-control');

    if (contentInput) {
      contentInput.addEventListener('input', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].content = contentInput.value;
        dirty = true;
        render();
      });
    }
    if (contentSelect) {
      contentSelect.addEventListener('change', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].content = contentSelect.value;
        dirty = true;
        render();
      });
    }
    if (colorInput) {
      colorInput.addEventListener('input', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].color = colorInput.value;
        dirty = true;
        render();
      });
    }
    var fontSelectEl = document.getElementById('prop-font');
    if (fontSelectEl) {
      fontSelectEl.addEventListener('change', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].font = fontSelectEl.value || 'default';
        dirty = true;
        render();
      });
    }
    var sizeSelectEl = document.getElementById('prop-text-size');
    if (sizeSelectEl) {
      sizeSelectEl.addEventListener('change', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        var v = parseInt(sizeSelectEl.value, 10);
        widgets[selectedIndex].text_size = (isNaN(v) || v < 1) ? 1 : v;
        dirty = true;
        render();
      });
    }
    var transformSelectEl = document.getElementById('prop-text-transform');
    if (transformSelectEl) {
      transformSelectEl.addEventListener('change', function() {
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        widgets[selectedIndex].text_transform = transformSelectEl.value || 'none';
        dirty = true;
        render();
      });
    }
    if (alignCtrl) {
      alignCtrl.addEventListener('click', function(e) {
        var t = e.target;
        if (!t || !t.classList || !t.classList.contains('align-btn')) return;
        if (selectedIndex < 0 || !widgets[selectedIndex]) return;
        if (widgets[selectedIndex].type !== 'text') return;
        var a = t.getAttribute('data-align');
        widgets[selectedIndex].align = a;
        dirty = true;
        updateAlignButtons(a);
        render();
      });
    }
  }

  /* ---------- Save helpers ---------- */
  function nameToId(name) {
    var id = name.replace(/[^A-Za-z0-9_-]+/g, '-')
                 .replace(/^-+|-+$/g, '')
                 .substring(0, 32);
    if (!id) id = 'layout-' + Date.now();
    return id;
  }

  function getLayoutBody() {
    var nameEl = document.getElementById('layout-name');
    var name = (nameEl && nameEl.value.trim()) ? nameEl.value.trim() : 'My Layout';
    /* Reuse stored id for PUT; derive from name for new POST */
    var id = currentLayoutId ? currentLayoutId : nameToId(name);
    return {
      id: id,
      name: name,
      version: 1,
      canvas: { width: deviceW, height: deviceH },
      widgets: widgets
    };
  }

  function saveLayout(andActivate) {
    var nameEl = document.getElementById('layout-name');
    var name = nameEl ? nameEl.value.trim() : '';
    if (!name) {
      FW.showToast('Enter a layout name before saving', 'error');
      return;
    }
    if (name.length > 32) {
      FW.showToast('Layout name must be 32 characters or less', 'error');
      return;
    }
    var body = getLayoutBody();
    var savePromise;
    if (currentLayoutId) {
      /* PUT — overwrite existing */
      savePromise = FW.put('/api/layouts/' + currentLayoutId, body);
    } else {
      /* POST — create new */
      savePromise = FW.post('/api/layouts', body);
    }
    savePromise.then(function(res) {
      if (!res || !res.body || !res.body.ok) {
        var errMsg = (res && res.body && res.body.error) ? res.body.error : 'Unknown error';
        FW.showToast('Save failed \u2014 ' + errMsg, 'error');
        return null;
      }
      /* Store returned id (from POST response; PUT echoes the same id) */
      if (res.body.data && res.body.data.id) {
        currentLayoutId = res.body.data.id;
      }
      dirty = false;
      FW.showToast('Layout saved', 'success');
      if (!andActivate) return null;
      /* Chain activate */
      FW.showToast('Applying\u2026', 'info');
      return FW.post('/api/layouts/' + currentLayoutId + '/activate', {});
    }).then(function(res) {
      if (res === null || res === undefined) return;
      if (!res.body || !res.body.ok) {
        var errMsg = (res.body && res.body.error) ? res.body.error : 'Unknown error';
        FW.showToast('Activate failed \u2014 ' + errMsg, 'error');
        return;
      }
      FW.showToast('Layout active', 'success');
    }).catch(function() {
      FW.showToast('Network error \u2014 check device connection', 'error');
    });
  }

  function saveAsNew() {
    var nameEl = document.getElementById('layout-name');
    var name = nameEl ? nameEl.value.trim() : '';
    if (!name) {
      FW.showToast('Enter a layout name before saving', 'error');
      return;
    }
    /* Force a new id by clearing currentLayoutId */
    currentLayoutId = null;
    saveLayout(false);
  }

  function bindSaveButtons() {
    var btnNew      = document.getElementById('btn-save-new');
    var btnSave     = document.getElementById('btn-save');
    var btnActivate = document.getElementById('btn-activate');
    if (btnNew)      btnNew.addEventListener('click', saveAsNew);
    if (btnSave)     btnSave.addEventListener('click', function() { saveLayout(false); });
    if (btnActivate) btnActivate.addEventListener('click', function() { saveLayout(true); });
  }

  /* ---------- Unsaved-changes guard ---------- */
  function bindUnloadGuard() {
    window.addEventListener('beforeunload', function(e) {
      if (!dirty) return;
      e.preventDefault();
      /* returnValue required for Chrome/Edge legacy support */
      e.returnValue = 'You have unsaved changes. Leave anyway?';
    });
  }

  /* ---------- Init / load ---------- */
  function loadLayout() {
    FW.get('/api/layout').then(function(res) {
      if (!res.body || !res.body.ok) {
        FW.showToast('Cannot load layout info \u2014 check device connection', 'error');
        return null;
      }
      var d = res.body.data;
      /* Guard against malformed API response before destructuring nested fields */
      if (!d || !d.matrix || typeof d.matrix.width !== 'number' ||
          typeof d.matrix.height !== 'number' || !d.hardware ||
          typeof d.hardware.tile_pixels !== 'number') {
        FW.showToast('Invalid layout data \u2014 check device firmware', 'error');
        return null;
      }
      initCanvas(d.matrix.width, d.matrix.height, d.hardware.tile_pixels);
      return FW.get('/api/widgets/types');
    }).then(function(res) {
      if (!res || !res.body || !res.body.ok) return;
      var types = res.body.data;
      /* Guard against non-array response before iterating */
      if (!types || typeof types.length !== 'number') return;
      initToolbox(types);
      updateSnapButtons();
    }).catch(function(err) {
      /* Network transport failure (fetch rejected) — show error toast */
      FW.showToast('Editor failed to load \u2014 check device connection', 'error');
    });
  }

  /* Keyboard shortcuts for fine-grained positioning and quick deletes. Only
     fires when a widget is selected AND the focus isn't in a text/form
     field — typing in the layout-name input shouldn't nudge widgets.
     Arrow alone = 1px; Shift+arrow = SNAP px; Delete/Backspace = remove. */
  function isFormElement(el) {
    if (!el || !el.tagName) return false;
    var tag = el.tagName.toLowerCase();
    if (tag === 'input' || tag === 'select' || tag === 'textarea') return true;
    if (el.isContentEditable) return true;
    return false;
  }

  function onKeyDown(e) {
    if (selectedIndex < 0 || !widgets[selectedIndex]) return;
    if (isFormElement(e.target)) return;
    var w = widgets[selectedIndex];
    var step = e.shiftKey ? SNAP : 1;
    var moved = false;
    var deleted = false;
    if (e.key === 'ArrowLeft') {
      w.x = clamp(w.x - step, 0, deviceW - w.w); moved = true;
    } else if (e.key === 'ArrowRight') {
      w.x = clamp(w.x + step, 0, deviceW - w.w); moved = true;
    } else if (e.key === 'ArrowUp') {
      w.y = clamp(w.y - step, 0, deviceH - w.h); moved = true;
    } else if (e.key === 'ArrowDown') {
      w.y = clamp(w.y + step, 0, deviceH - w.h); moved = true;
    } else if (e.key === 'Delete' || e.key === 'Backspace') {
      widgets.splice(selectedIndex, 1);
      // Remap any remaining group selections: drop the deleted index and
      // shift anything above it down by one.
      var remapped = [];
      for (var sI = 0; sI < selectedIndices.length; sI++) {
        var si = selectedIndices[sI];
        if (si === selectedIndex) continue;
        remapped.push(si > selectedIndex ? si - 1 : si);
      }
      selectedIndices = remapped;
      selectedIndex = -1;
      deleted = true;
    }
    if (moved || deleted) {
      dirty = true;
      e.preventDefault();
      render();
      if (deleted) showPropsPanel(null);
      else showPropsPanel(widgets[selectedIndex]);
      setStatus(deleted ? 'widget deleted' :
        w.type + ' @ ' + w.x + ',' + w.y + ' ' + w.w + 'x' + w.h);
    }
  }

  function bindCanvasEvents() {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    canvas.addEventListener('pointerdown', onPointerDown);
    canvas.addEventListener('pointermove', onPointerMove);
    canvas.addEventListener('pointerup', onPointerUp);
    document.addEventListener('keydown', onKeyDown);
    canvas.addEventListener('pointercancel', onPointerUp);
  }

  /* ---------- Live preview plumbing ---------- */
  /* Pull the latest enriched flight list every PREVIEW_POLL_MS. We keep
     *all* flights so the cycle timer can rotate through them — showing
     only flights[0] made the preview feel stuck on a single aircraft when
     the sky had several. Preserves the current cycle index across polls
     when the active flight is still visible, so a poll mid-cycle doesn't
     reset you to flight #1. */
  function refreshPreviewFlight() {
    if (!previewLiveEnabled) return;
    window.FW.get('/api/flights/current').then(function (res) {
      if (!res || !res.body || !res.body.ok || !res.body.data) return;
      var list = res.body.data.flights || [];

      // Try to keep showing the same flight after the poll, matching by
      // ident when possible. If it's gone, stay at the same numeric index
      // (modulo'd in activePreviewFlight) so the cycle stays steady.
      var priorIdent = null;
      if (previewFlights.length > 0 && previewFlightIndex < previewFlights.length) {
        var prior = previewFlights[previewFlightIndex];
        priorIdent = prior && prior.ident;
      }
      previewFlights = list;
      if (priorIdent) {
        for (var i = 0; i < list.length; i++) {
          if (list[i] && list[i].ident === priorIdent) {
            previewFlightIndex = i;
            break;
          }
        }
      }
      renderPreview();
    }).catch(function () { /* keep last snapshot on transient errors */ });
  }

  /* Advance to the next flight in the rotation. Called on a timer so the
     preview mirrors the wall's own cycling when the sky has multiple
     aircraft. Single-flight skies are no-ops because idx always wraps to 0. */
  function cyclePreviewFlight() {
    if (!previewLiveEnabled) return;
    if (previewFlights.length <= 1) return;
    previewFlightIndex = (previewFlightIndex + 1) % previewFlights.length;
    renderPreview();
  }

  function startPreviewPolling() {
    if (previewPollTimer === null) {
      refreshPreviewFlight();
      previewPollTimer = setInterval(refreshPreviewFlight, PREVIEW_POLL_MS);
    }
    if (previewCycleTimer === null) {
      previewCycleTimer = setInterval(cyclePreviewFlight, PREVIEW_CYCLE_MS);
    }
  }

  function stopPreviewPolling() {
    if (previewPollTimer !== null) {
      clearInterval(previewPollTimer);
      previewPollTimer = null;
    }
    if (previewCycleTimer !== null) {
      clearInterval(previewCycleTimer);
      previewCycleTimer = null;
    }
  }

  /* Redistribute all existing widgets vertically with the same EDGE_MARGIN /
     WIDGET_GAP used for auto-placement of new widgets. Preserves each widget's
     original width/height/color/content/type — we only touch x and y. Stack
     order follows current .y then .x so a user's intuitive top-to-bottom
     reading order survives the reshuffle. */
  function autoSpaceWidgets() {
    if (!widgets.length) return;
    var sorted = widgets.slice().sort(function (a, b) {
      if (a.y !== b.y) return a.y - b.y;
      return a.x - b.x;
    });
    var cursorY = snapTo(EDGE_MARGIN, SNAP);
    for (var i = 0; i < sorted.length; i++) {
      var w = sorted[i];
      var x = snapTo(EDGE_MARGIN, SNAP);
      var y = cursorY;
      if (y + w.h + EDGE_MARGIN > deviceH) {
        // Out of vertical room — leave overflow widgets where they were
        // rather than clobbering or silently dropping them.
        continue;
      }
      w.x = clamp(x, 0, deviceW - w.w);
      w.y = clamp(y, 0, deviceH - w.h);
      cursorY = w.y + w.h + WIDGET_GAP;
    }
    dirty = true;
    render();
    if (selectedIndex >= 0 && widgets[selectedIndex]) {
      showPropsPanel(widgets[selectedIndex]);
    }
    setStatus('Widgets spaced vertically');
  }

  function bindAutoSpace() {
    var btn = document.getElementById('btn-auto-space');
    if (btn) btn.addEventListener('click', autoSpaceWidgets);
  }

  /* Group-align operations. Each kind translates to an axis + edge/center
     computation over the selected group's bounding box. Noop when fewer
     than two widgets are selected — single-widget alignment against itself
     isn't useful. Clamps results to matrix bounds so an align can never
     push a widget off-canvas. */
  function applyGroupAlign(kind) {
    if (!selectedIndices || selectedIndices.length < 2) {
      setStatus('Select 2+ widgets to align (shift-click)');
      return;
    }
    // Bounding box of the group.
    var minL = Infinity, minT = Infinity;
    var maxR = -Infinity, maxB = -Infinity;
    for (var i = 0; i < selectedIndices.length; i++) {
      var w = widgets[selectedIndices[i]];
      if (!w) continue;
      if (w.x < minL) minL = w.x;
      if (w.y < minT) minT = w.y;
      if (w.x + w.w > maxR) maxR = w.x + w.w;
      if (w.y + w.h > maxB) maxB = w.y + w.h;
    }
    var centerX = (minL + maxR) / 2;
    var centerY = (minT + maxB) / 2;
    for (var j = 0; j < selectedIndices.length; j++) {
      var w2 = widgets[selectedIndices[j]];
      if (!w2) continue;
      if      (kind === 'left')    w2.x = minL;
      else if (kind === 'right')   w2.x = maxR - w2.w;
      else if (kind === 'centerX') w2.x = (centerX - (w2.w / 2)) | 0;
      else if (kind === 'top')     w2.y = minT;
      else if (kind === 'bottom')  w2.y = maxB - w2.h;
      else if (kind === 'centerY') w2.y = (centerY - (w2.h / 2)) | 0;
      w2.x = clamp(w2.x, 0, deviceW - w2.w);
      w2.y = clamp(w2.y, 0, deviceH - w2.h);
    }
    dirty = true;
    render();
    if (selectedIndex >= 0 && widgets[selectedIndex]) {
      showPropsPanel(widgets[selectedIndex]);
    }
    setStatus('Aligned ' + selectedIndices.length + ' widgets (' + kind + ')');
  }

  function bindGroupAlign() {
    var container = document.getElementById('align-group');
    if (!container) return;
    container.addEventListener('click', function (e) {
      var t = e.target;
      if (!t || !t.getAttribute) return;
      var kind = t.getAttribute('data-group-align');
      if (kind) applyGroupAlign(kind);
    });
  }

  function bindPreviewControls() {
    var toggle = document.getElementById('preview-live-data');
    if (!toggle) return;
    toggle.addEventListener('change', function () {
      previewLiveEnabled = !!toggle.checked;
      if (previewLiveEnabled) {
        startPreviewPolling();
      } else {
        stopPreviewPolling();
      }
      renderPreview();
    });
  }

  function init() {
    bindCanvasEvents();
    bindPropsPanelEvents();
    bindSaveButtons();
    bindUnloadGuard();
    initSnap();
    bindPreviewControls();
    bindAutoSpace();
    bindGroupAlign();
    loadLayout();
    startPreviewPolling();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
