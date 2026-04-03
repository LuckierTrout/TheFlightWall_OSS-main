/* FlightWall shared HTTP helpers */
var FW = (function() {
  'use strict';

  function parseJsonResponse(res) {
    return res.text().then(function(text) {
      if (!text) {
        return {
          status: res.status,
          body: { ok: false, error: 'Empty server response (HTTP ' + res.status + ')', code: 'EMPTY_RESPONSE' }
        };
      }

      try {
        return { status: res.status, body: JSON.parse(text) };
      } catch (err) {
        return {
          status: res.status,
          body: { ok: false, error: 'Invalid server response (HTTP ' + res.status + ')', code: 'INVALID_RESPONSE' }
        };
      }
    });
  }

  function fetchJson(url, opts) {
    return fetch(url, opts || {}).then(function(res) {
      return parseJsonResponse(res);
    });
  }

  function get(url) {
    return fetchJson(url);
  }

  function post(url, data) {
    return fetchJson(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    });
  }

  function del(url) {
    return fetchJson(url, { method: 'DELETE' });
  }

  /* Toast notification system */
  var toastContainer = null;

  function ensureToastContainer() {
    if (toastContainer) return toastContainer;
    toastContainer = document.getElementById('toast-container');
    if (!toastContainer) {
      toastContainer = document.createElement('div');
      toastContainer.id = 'toast-container';
      document.body.appendChild(toastContainer);
    }
    return toastContainer;
  }

  function showToast(message, severity) {
    var container = ensureToastContainer();
    var toast = document.createElement('div');
    toast.className = 'toast toast-' + (severity || 'success');
    toast.textContent = message;
    container.appendChild(toast);
    // Trigger reflow for animation
    toast.offsetHeight;
    toast.classList.add('toast-visible');
    setTimeout(function() {
      toast.classList.remove('toast-visible');
      toast.classList.add('toast-exit');
      setTimeout(function() {
        if (toast.parentNode) toast.parentNode.removeChild(toast);
      }, 300);
    }, 2500);
  }

  return { get: get, post: post, del: del, showToast: showToast };
})();
