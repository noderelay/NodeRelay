/* Shared script for the docs pages (index.html, quality.html, howto.html).
   The theme-restoring snippet stays inline in each page's <head> since it
   must run before first paint to avoid a light/dark flash. */

/* ── theme cycler ──
   Cycles through a few palettes lifted from the client's shipped themes.
   "light"/"dark" keep their old names so stored preferences carry over. */
(function() {
  const btn = document.getElementById('themeToggle');
  if (!btn) return;
  const root = document.documentElement;
  const THEMES = ['dark', 'light', 'nord', 'catppuccin'];
  const NAMES = {
    dark: 'Gruvbox Dark', light: 'Gruvbox Light',
    nord: 'Nord', catppuccin: 'Catppuccin Mocha'
  };
  function label() {
    const cur = root.getAttribute('data-theme');
    btn.title = 'Theme: ' + (NAMES[cur] || cur) + ' (click to cycle)';
  }
  label();
  btn.addEventListener('click', () => {
    const cur = root.getAttribute('data-theme');
    const next = THEMES[(THEMES.indexOf(cur) + 1) % THEMES.length];
    root.setAttribute('data-theme', next);
    localStorage.setItem('uplink-theme', next);
    label();
  });
})();

/* ── random app icon rotation ── */
(function() {
  const icons = [
    'black-old-orange','black-orange','colorful-blueish','colorful-greenblue',
    'colorful-hotbluepink','colorful-orange','colorful-purple','flat-black',
    'gruvbox-blue','gruvbox-colorful','gruvbox-orange','gruvbox-purple',
    'gruvbox-yellow','original-black','original-flat-shine'
  ];
  const pick = icons[Math.floor(Math.random() * icons.length)];
  document.querySelectorAll('[data-uplink-icon]').forEach(function(img) {
    img.src = 'assets/icons/' + pick + '.png';
  });
})();
