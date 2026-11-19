#ifndef PORTAL_ASSETS_H
#define PORTAL_ASSETS_H

#include <Arduino.h>
#include "PortalRbJs.h"

const char PORTAL_CSS[] PROGMEM = R"rawliteral(
  * { box-sizing: border-box; }
  body { font-family: Arial, sans-serif; background: #121212; color: #eee; margin: 0; padding: 16px; }
  .wrap { max-width: 1120px; margin: auto; }
  .topbar { display: flex; flex-wrap: wrap; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 16px; border-bottom: 1px solid #333; padding-bottom: 12px; }
  .topbar h1 { color: #4fc3f7; font-size: 20px; margin: 0; }
  .tabs { display: flex; flex-wrap: wrap; gap: 8px; }
  .tab { padding: 10px 16px; background: #263238; color: #ccc; border: 1px solid #37474f; border-radius: 6px; cursor: pointer; font-size: 14px; font-weight: bold; }
  .tab.active { background: #0288d1; color: #fff; border-color: #4fc3f7; }
  .pane { display: none; }
  .pane.active { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 16px; }
  .stack { display: flex; flex-direction: column; gap: 16px; }
  .card { background: #1e1e1e; padding: 18px; border-radius: 8px; border: 1px solid #2c2c2c; }
  .card h2 { color: #4fc3f7; font-size: 17px; margin: 0 0 12px 0; }
  label { display: block; margin-top: 12px; font-size: 13px; color: #bbb; font-weight: bold; }
  .hint { font-size: 12px; color: #888; margin: 4px 0 0 0; }
  select, input[type=text], input[type=password], input[type=file] { width: 100%; padding: 10px; margin-top: 5px; border-radius: 4px; border: 1px solid #444; background: #2c2c2c; color: #fff; font-size: 14px; }
  .row { display: flex; gap: 8px; margin-top: 6px; }
  .btn { width: 100%; padding: 11px; margin-top: 14px; background: #0288d1; color: #fff; border: none; border-radius: 4px; font-size: 14px; font-weight: bold; cursor: pointer; text-align: center; text-decoration: none; display: inline-block; }
  .btn:disabled { opacity: 0.35; cursor: not-allowed; }
  .btn-green { background: #2e7d32; } .btn-teal { background: #00897b; } .btn-orange { background: #ef6c00; }
  .btn-slate { background: #37474f; margin-top: 8px; padding: 8px; font-size: 13px; }
  .btn-inline { width: auto; margin-top: 5px; padding: 10px 14px; white-space: nowrap; }
  .aud-bar { margin-top: 12px; padding: 12px; background: #16262e; border: 1px solid #0288d1; border-radius: 6px; }
  .aud-title { font-size: 13px; font-weight: bold; color: #4fc3f7; margin-bottom: 6px; }
  .warn-banner { margin: 8px 0; padding: 8px 10px; background: #3e2723; border: 1px solid #ff7043; border-radius: 4px; color: #ffccbc; font-size: 12px; font-weight: bold; }
  table { width: 100%; border-collapse: collapse; font-size: 13px; }
  td { padding: 8px 6px; border-bottom: 1px solid #333; vertical-align: middle; }
  .fav-acts { display: flex; gap: 4px; justify-content: flex-end; align-items: center; }
  .btn-mv { background: #263238; color: #eee; border: 1px solid #455a64; padding: 4px 8px; border-radius: 4px; cursor: pointer; font-weight: bold; }
  .btn-mv:disabled { opacity: 0.25; cursor: not-allowed; }
  .del { color: #ff5252; text-decoration: none; font-weight: bold; padding: 4px 8px; border: 1px solid #5c2b2b; border-radius: 4px; background: transparent; cursor: pointer; }
)rawliteral";

const char PORTAL_JS[] PROGMEM = R"rawliteral(
<script>
let bbcList = [];
let favsFull = false;
function escHtml(s) {
  return String(s || '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}
function showTab(id, btn) {
  document.querySelectorAll('.pane').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  if (btn) btn.classList.add('active');
  localStorage.setItem('radioTab', id);
}
function updateAppendAvailability() {
  const appBtn = document.getElementById('btnRbAppend');
  const banner = document.getElementById('rbFullBanner');
  if (appBtn) appBtn.disabled = favsFull;
  if (banner) banner.style.display = favsFull ? 'block' : 'none';
}
function renderBbc(items) {
  const sel = document.getElementById('bbcSel');
  sel.innerHTML = items.length ? items.map((s, i) => `<option value="${i}">${escHtml(s.name)}</option>`).join('') : '<option value="">No matching stations</option>';
  sel._items = items;
}
function filterBbc() {
  const q = document.getElementById('q').value.toLowerCase();
  renderBbc(bbcList.filter(s => s.name.toLowerCase().includes(q)));
}
async function loadFavs() {
  try {
    const r = await fetch('/fav_list');
    const favs = await r.json();
    const activeCount = favs.filter(f => f && f.name && f.name.length > 0).length;
    let firstEmpty = -1;
    document.getElementById('favTbl').innerHTML = favs.map((f, i) => {
      const ok = f && f.name && f.name.length > 0;
      if (!ok && firstEmpty === -1) firstEmpty = i;
      const acts = ok ? `<div class="fav-acts"><button class="btn-mv" ${i===0?'disabled':''} onclick="moveFav(${i},-1)">&uarr;</button><button class="btn-mv" ${i>=activeCount-1?'disabled':''} onclick="moveFav(${i},1)">&darr;</button><button class="del" onclick="clearFav(${i})">Del</button></div>` : '';
      return `<tr><td style="width:40px;"><strong>#${i + 1}</strong></td><td>${ok ? escHtml(f.name) : '<em style="color:#777;">Empty Slot</em>'}</td><td style="text-align:right;">${acts}</td></tr>`;
    }).join('');
    favsFull = (firstEmpty === -1);
    updateAppendAvailability();
    const slotSel = document.getElementById('favSlotSel');
    if (slotSel && firstEmpty !== -1) slotSel.value = String(firstEmpty);
  } catch (e) {}
}
async function loadBbc() {
  try { const r = await fetch('/bbc_search?q='); bbcList = await r.json(); renderBbc(bbcList); }
  catch (e) { document.getElementById('bbcSel').innerHTML = '<option value="">Failed to load BBC Gist</option>'; }
}
async function saveSelected() {
  const sel = document.getElementById('bbcSel');
  const item = sel._items && sel._items[sel.value];
  if (!item) return;
  await fetch('/fav_save?slot=' + document.getElementById('favSlotSel').value + '&name=' + encodeURIComponent(item.name) + '&url=' + encodeURIComponent(item.url), {method:'POST'});
  loadFavs();
}
async function moveFav(slot, dir) { await fetch('/fav_move?slot=' + slot + '&dir=' + dir); loadFavs(); }
async function clearFav(slot) { await fetch('/fav_clear?slot=' + slot); loadFavs(); }
window.onload = () => {
  const saved = localStorage.getItem('radioTab');
  if (saved && document.getElementById(saved)) showTab(saved, document.querySelector(`.tab[data-tab="${saved}"]`));
  loadFavs(); loadBbc(); onRbModeChange(); syncAuditionState();
};
</script>
)rawliteral";

#endif
// END OF FILE
