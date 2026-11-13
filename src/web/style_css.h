#pragma once
#include <pgmspace.h>

// Gaya dashboard. Disajikan di "/style.css".
// Skala tetap: font 12/14/16/32/48, spacing kelipatan 4, radius 12.
// Pastel per tile (pilihan pemilik): satu warna per jenis perangkat supaya cepat dikenali.
// Setiap tile punya --tint (latar), --deep (lingkaran ikon), --on (lingkaran saat aktif),
// --ink (teks status, kontras >= 4.5:1 di atas --tint). Merah tegas hanya untuk bahaya api.
const char STYLE_CSS[] PROGMEM = R"rawliteral(
:root{
  --bg:#FBF7F2;--tile:#FFFFFF;--line:#ECE6DE;--soft:#F3EEE7;
  --text:#1F2328;--muted:#5A606B;--grey:#8A9099;
  --accent:#8A3F12;--red:#C9302C;--red-2:#B42A26;
  --r:12px;--t:180ms;
  --font:system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;
}
*{box-sizing:border-box;margin:0;padding:0}
html{-webkit-text-size-adjust:100%}
body{background:var(--bg);color:var(--text);font:400 14px/1.4 var(--font);min-height:100vh}
button{font:inherit;color:inherit;background:none;border:0}
button,label,[role=tab]{cursor:pointer}
button:disabled,button[aria-disabled=true]{cursor:not-allowed}
:focus-visible{outline:2px solid var(--text);outline-offset:2px}
[hidden]{display:none!important}
.sr{position:absolute;width:1px;height:1px;overflow:hidden;clip:rect(0 0 0 0);white-space:nowrap}
svg{width:24px;height:24px;fill:none;stroke:currentColor;stroke-width:1.5;stroke-linecap:round;stroke-linejoin:round}

/* Top bar */
.bar{position:sticky;top:0;z-index:5;display:flex;align-items:center;justify-content:space-between;gap:16px;
  max-width:960px;margin:0 auto;padding:16px;background:var(--bg)}
.bar h1{font-size:16px;font-weight:600}
.conn{font-size:12px;color:var(--muted);font-variant-numeric:tabular-nums}
.conn.bad{color:var(--red);font-weight:600}

/* Grid tile */
.main{max-width:960px;margin:0 auto;padding:0 16px 104px}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}
@media (min-width:640px){.grid{grid-template-columns:repeat(3,minmax(0,1fr))}}
@media (min-width:880px){.grid{grid-template-columns:repeat(4,minmax(0,1fr))}}
.tile{--tint:var(--tile);--deep:var(--soft);--on:var(--soft);--ink:var(--text);
  position:relative;background:var(--tint);border:1px solid rgba(31,35,40,.06);border-radius:var(--r);
  transition:background-color var(--t),border-color var(--t),color var(--t)}
.tile.wide{grid-column:span 2}
.hit{display:flex;flex-direction:column;align-items:center;width:100%;min-height:160px;padding:12px 12px 16px;text-align:center;border-radius:var(--r)}
#tDoor{--tint:#FFE9DC;--deep:#FFD6BF;--on:#F6A97C;--ink:#8A3F12}
#tMode{--tint:#EEE8FB;--deep:#DDD3F6;--on:#B6A2EC;--ink:#523F9E}
#tFlame{--tint:#FCE6EC;--deep:#F7CFDA;--on:#F2A9BD;--ink:#9B2C4A}
#tDist{--tint:#E2F1FB;--deep:#C8E4F6;--on:#9ACDEE;--ink:#1E5F8C}
#tTemp{--tint:#E0F4F2;--deep:#C4ECE8;--on:#82D5CC;--ink:#12635B}
@media (hover:hover){button.hit:hover:not(:disabled){background:var(--deep)}}
button.hit:active:not(:disabled) .ic{transform:scale(.94)}
.t-title{align-self:flex-start;font-size:12px;color:var(--muted)}
.ic{display:grid;place-items:center;width:56px;height:56px;margin:12px 0 8px;border-radius:50%;
  background:var(--deep);color:var(--ink);transition:background-color var(--t),color var(--t),transform var(--t)}
.ic svg{width:28px;height:28px}
.t-val{font-size:16px;font-weight:600}
.t-sub{font-size:12px;color:var(--muted);min-height:16px;margin-top:4px}
.tile.on .ic{background:var(--on);color:var(--text)}
.tile.on .t-sub{color:var(--ink);font-weight:600}
.tile .t-sub.alert{color:var(--red);font-weight:600}
.tile[aria-busy=true] .hit{opacity:.6}

.acts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;width:100%;margin-top:16px}
.acts .btn{min-width:0;padding:0 8px;white-space:nowrap;background:#fff;border-color:rgba(138,63,18,.25)}
@media (hover:hover){.acts .btn:hover:not(:disabled){background:#FFF6F0}}
.acts .btn:active:not(:disabled){background:var(--deep)}
#bAuto{grid-column:1/-1}

/* Angka besar */
.num{display:flex;align-items:baseline;gap:4px;margin:12px 0 8px;font-size:48px;font-weight:300;line-height:56px;font-variant-numeric:tabular-nums}
.num small{font-size:14px;color:var(--muted)}
.gauge{position:relative;width:100%;height:6px;border-radius:3px;background:var(--deep);overflow:hidden}
.fill{position:absolute;inset:0 auto 0 0;width:0;background:var(--on);transition:width var(--t)}
.tile.on .fill{background:var(--ink)}
.tick{position:absolute;top:0;bottom:0;left:50%;width:2px;background:var(--ink)}
#tDist .t-sub,#tTemp .t-sub{margin-top:8px}

/* Api: bahaya */
.tile.danger{background:var(--red);border-color:var(--red);color:#fff}
.tile.danger .t-title,.tile.danger .t-sub{color:#fff}
.tile.danger .ic{background:rgba(255,255,255,.2);color:#fff}
.tile.danger.pulse{animation:pulse 1.6s ease-in-out 3}
.tile .alarm-btn{margin-top:12px;min-width:0;padding:0 12px;white-space:nowrap;background:#fff;color:var(--red);border-color:#fff;font-weight:600}
@media (hover:hover){.tile .alarm-btn:hover:not(:disabled){background:#FCE6EC}}
.tile .alarm-btn:active:not(:disabled){background:#F7CFDA}
@keyframes pulse{50%{background:var(--red-2);border-color:var(--red-2)}}

/* Offline: semua tile abu-abu */
.offline .tile{background:#EEF0F2;color:var(--muted)}
.offline .tile .ic{background:#E3E5E8;color:var(--grey)}
.offline .tile .t-title,.offline .tile .t-sub{color:var(--muted)}
.offline .tile .fill{background:var(--grey)}
.offline .tile.pulse{animation:none}

/* Daftar (Pengaturan, Info) */
.v-title{font-size:16px;font-weight:600;margin:4px 0 12px}
.list{list-style:none;background:var(--tile);border:1px solid var(--line);border-radius:var(--r)}
.list li,.list div{display:flex;align-items:center;justify-content:space-between;gap:12px;min-height:56px;padding:8px 16px}
.list li+li,.list div+div{border-top:1px solid var(--line)}
.list b{display:block;font-weight:500}
.list small{display:block;font-size:12px;color:var(--muted)}
.list dt{color:var(--muted)}
.list dd{font-weight:500;text-align:right;font-variant-numeric:tabular-nums;overflow-wrap:anywhere}
.btn{min-width:72px;min-height:44px;padding:0 16px;border:1px solid var(--line);border-radius:var(--r);font-weight:500;transition:background-color var(--t)}
.btn:hover:not(:disabled){background:var(--soft)}
.btn:active:not(:disabled){background:var(--line)}
.btn:disabled{color:var(--grey)}

/* Navigasi bawah */
.tabs{position:fixed;left:0;right:0;bottom:0;z-index:6;display:flex;justify-content:center;
  background:var(--tile);border-top:1px solid var(--line);padding-bottom:env(safe-area-inset-bottom)}
.tabs [role=tab]{flex:1;max-width:200px;display:flex;flex-direction:column;align-items:center;gap:4px;min-height:64px;padding:8px 4px;font-size:12px;color:var(--muted);transition:color var(--t)}
.tabs [role=tab][aria-selected=true]{color:var(--text);font-weight:600}
.tabs [role=tab][aria-selected=true] svg{color:var(--accent)}
.tabs [role=tab]:hover{color:var(--text)}

/* Toast */
.toast{position:fixed;left:50%;bottom:80px;z-index:7;transform:translate(-50%,8px);max-width:calc(100% - 32px);
  padding:12px 16px;border-radius:var(--r);background:var(--text);color:#fff;font-size:14px;
  opacity:0;pointer-events:none;transition:opacity var(--t),transform var(--t)}
.toast.show{opacity:1;transform:translate(-50%,0)}

/* Bottom sheet */
.sheet{position:fixed;inset:auto 0 0;margin:0 auto;width:100%;max-width:480px;max-height:none;border:0;
  border-radius:var(--r) var(--r) 0 0;background:var(--tile);color:var(--text);
  padding:16px 16px calc(16px + env(safe-area-inset-bottom))}
.sheet[open]{animation:up var(--t) ease-out}
.sheet::backdrop{background:rgba(31,35,40,.4)}
@keyframes up{from{transform:translateY(24px);opacity:0}}
.sheet h2{font-size:16px;font-weight:600;margin-bottom:8px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;width:100%;min-height:52px;padding:0 4px;font-size:16px;text-align:left;border-top:1px solid var(--line)}
.row:hover:not(:disabled){background:var(--soft)}
.row:disabled{color:var(--grey)}
.row.cancel{justify-content:center;color:var(--muted);margin-top:8px}
.row input{width:20px;height:20px;accent-color:var(--accent)}

@media (prefers-reduced-motion:reduce){
  *{transition:none!important;animation:none!important}
}
)rawliteral";
