/* FlightWall Layout Editor (LE-1.7) - ES5 only */
(function() {
  'use strict';

  /* ---------- Module globals ---------- */
  var SCALE = 4;                /* CSS pixels per logical pixel */
  var SNAP = 8;                 /* current snap grid: 8 / 16 / 1 */
  var deviceW = 0;              /* logical canvas width (from /api/layout) */
  var deviceH = 0;              /* logical canvas height */
  var tilePixels = 16;          /* tile size in logical pixels */
  var widgets = [];             /* [{type,x,y,w,h,color,content,align,id}] */
  var selectedIndex = -1;       /* -1 = no selection */
  var dragState = null;         /* {mode,startX,startY,origX,origY,origW,origH,toastedFloor} */
  var widgetTypeMeta = {};      /* keyed by type string */
  var HANDLE_SIZE = 6;          /* CSS pixels for resize handle */
  var dirty = false;            /* true when widgets changed since last save */
  var currentLayoutId = null;   /* string id after first save, or null for new */

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
    /* 3. Widgets */
    for (var i = 0; i < widgets.length; i++) {
      drawWidget(ctx, widgets[i], i === selectedIndex);
    }
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

  function drawWidget(ctx, w, isSelected) {
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
    /* Selection outline + resize handle */
    if (isSelected) {
      ctx.strokeStyle = '#FFF';
      ctx.lineWidth = 1;
      ctx.strokeRect(x + 0.5, y + 0.5, cw - 1, ch - 1);
      ctx.fillStyle = '#FFF';
      ctx.fillRect(x + cw - HANDLE_SIZE, y + ch - HANDLE_SIZE, HANDLE_SIZE, HANDLE_SIZE);
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
    /* 1. Resize handle of selected widget */
    if (selectedIndex >= 0 && selectedIndex < widgets.length) {
      var sw = widgets[selectedIndex];
      var rx = sw.x * SCALE + sw.w * SCALE - HANDLE_SIZE;
      var ry = sw.y * SCALE + sw.h * SCALE - HANDLE_SIZE;
      if (cx >= rx && cx <= rx + HANDLE_SIZE && cy >= ry && cy <= ry + HANDLE_SIZE) {
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

  /* ---------- Pointer events ---------- */
  function onPointerDown(e) {
    var canvas = document.getElementById('editor-canvas');
    var p = getCanvasPos(e);
    var hit = hitTest(p.x, p.y);
    if (hit) {
      selectedIndex = hit.index;
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
      e.preventDefault();
    } else {
      selectedIndex = -1;
      dragState = null;
    }
    render();
    showPropsPanel(selectedIndex >= 0 ? widgets[selectedIndex] : null);
  }

  function onPointerMove(e) {
    if (dragState === null) return;
    var w = widgets[selectedIndex];
    if (!w) { dragState = null; return; }
    var dxCss = e.clientX - dragState.startX;
    var dyCss = e.clientY - dragState.startY;
    var dx = dxCss / SCALE;
    var dy = dyCss / SCALE;

    if (dragState.mode === 'move') {
      var nx = snapTo(dragState.origX + dx, SNAP);
      var ny = snapTo(dragState.origY + dy, SNAP);
      w.x = clamp(nx, 0, deviceW - w.w);
      w.y = clamp(ny, 0, deviceH - w.h);
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
      minW: 6, minH: 6,
      contentKind: 'string',   /* 'string' | 'select' — drives the props panel */
      contentOptions: null,    /* [ {value, label} ] when contentKind==='select' */
      contentLabel: 'Value'    /* props-panel label override (LE-1.10) */
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
        if (fo.unit) labelText = labelText + ' (' + fo.unit + ')';
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

  function addWidget(type) {
    var meta = widgetTypeMeta[type];
    if (!meta) return;
    var w = {
      type: type,
      x: snapTo(8, SNAP),
      y: snapTo(8, SNAP),
      w: meta.defaultW,
      h: meta.defaultH,
      color: meta.defaultColor || '#FFFFFF',
      content: meta.defaultContent || '',
      align: meta.defaultAlign || 'left',
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

  function bindCanvasEvents() {
    var canvas = document.getElementById('editor-canvas');
    if (!canvas) return;
    canvas.addEventListener('pointerdown', onPointerDown);
    canvas.addEventListener('pointermove', onPointerMove);
    canvas.addEventListener('pointerup', onPointerUp);
    canvas.addEventListener('pointercancel', onPointerUp);
  }

  function init() {
    bindCanvasEvents();
    bindPropsPanelEvents();
    bindSaveButtons();
    bindUnloadGuard();
    initSnap();
    loadLayout();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
