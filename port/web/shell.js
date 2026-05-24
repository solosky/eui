(function () {
  'use strict';

  var KEY_MAP = {
    ArrowUp:    0, ArrowDown:  1, ArrowLeft: 2,
    ArrowRight: 3, Enter:      4, Backspace: 5,
  };
  var EVT_PRESS = 0, EVT_RELEASE = 1;

  function push(type, keyIndex) {
    try { Module.ccall('eui_web_push_key_event', null,
                       ['number', 'number'], [type, keyIndex]); }
    catch (e) { /* Module not ready yet */ }
  }

  document.addEventListener('keydown', function (e) {
    if (e.key in KEY_MAP) { e.preventDefault(); push(EVT_PRESS, KEY_MAP[e.key]); }
  });
  document.addEventListener('keyup', function (e) {
    if (e.key in KEY_MAP) { e.preventDefault(); push(EVT_RELEASE, KEY_MAP[e.key]); }
  });

  var DBTN = {
    'btn-up': 0, 'btn-down': 1, 'btn-left': 2,
    'btn-right': 3, 'btn-ok': 4, 'btn-back': 5,
  };
  Object.keys(DBTN).forEach(function (id) {
    var el = document.getElementById(id);
    if (!el) return;
    var code = DBTN[id];
    el.addEventListener('touchstart', function (e) { e.preventDefault(); push(EVT_PRESS, code); });
    el.addEventListener('touchend',   function (e) { e.preventDefault(); push(EVT_RELEASE, code); });
    el.addEventListener('mousedown',  function (e) { e.preventDefault(); push(EVT_PRESS, code); });
    el.addEventListener('mouseup',    function (e) { e.preventDefault(); push(EVT_RELEASE, code); });
  });

  if (location.search.indexOf('fps=1') !== -1) {
    var fpsEl = document.getElementById('fps');
    if (fpsEl) {
      fpsEl.style.display = 'block';
      var frames = 0, last = performance.now();
      var origRaf = window.requestAnimationFrame;
      window.requestAnimationFrame = function (cb) { frames++; return origRaf(cb); };
      setInterval(function () {
        var now = performance.now();
        fpsEl.textContent = 'FPS: ' + Math.round(frames * 1000 / (now - last));
        frames = 0; last = now;
      }, 1000);
    }
  }
})();
