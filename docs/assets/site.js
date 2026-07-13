/* Shared script for the docs pages (index.html, quality.html, howto.html).
   The theme-restoring snippet stays inline in each page's <head> since it
   must run before first paint to avoid a light/dark flash. */

/* ── dark / light toggle ── */
(function() {
  const btn = document.getElementById('themeToggle');
  if (!btn) return;
  btn.addEventListener('click', () => {
    const root = document.documentElement;
    const next = root.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
    root.setAttribute('data-theme', next);
    localStorage.setItem('uplink-theme', next);
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
