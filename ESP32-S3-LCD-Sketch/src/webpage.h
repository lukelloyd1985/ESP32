#pragma once
// Phone-side page served by the board. Photos are resized, cropped and converted in the browser,
// so the board only ever receives screen-sized raw data.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sketch to Screen</title>
<style>
body{font-family:system-ui,sans-serif;margin:16px;max-width:520px}
canvas{width:100%;border:1px solid #888;background:#fff}
button,select,label{font-size:16px;margin:4px 4px 4px 0}
button{padding:8px 12px}
.row{margin:8px 0}
#msg{min-height:1.4em;color:#06c}
</style></head><body>
<h2>Sketch to Screen</h2>
<div class="row"><input type="file" id="file" accept="image/*" capture="environment"></div>
<canvas id="cv"></canvas>
<div class="row">
 <label><input type="radio" name="kind" value="art" checked> Line-art animation</label>
 <label><input type="radio" name="kind" value="pic"> Picture</label>
</div>
<div class="row" id="artOpts">
 <label>Sensitivity <input type="range" id="sens" min="2" max="40" value="14"></label>
 <label><input type="checkbox" id="inv"> Light ink on dark paper</label>
</div>
<div class="row"><button id="send">Send to device</button></div>
<div id="msg"></div>
<div class="row">Show on device:
 <button data-m="0">Picture</button><button data-m="1">Boil</button>
 <button data-m="2">Rainbow</button><button data-m="3">Draw-on</button>
</div>
<script>
const $=id=>document.getElementById(id), cv=$('cv'), ctx=cv.getContext('2d',{willReadFrequently:true});
let W=0,H=0,bmp=null,out=null;
const say=t=>$('msg').textContent=t;
fetch('/info').then(r=>r.json()).then(j=>{W=j.w;H=j.h;cv.width=W;cv.height=H;ctx.fillStyle='#fff';ctx.fillRect(0,0,W,H)});
const kind=()=>document.querySelector('input[name=kind]:checked').value;

$('file').onchange=async e=>{ if(!e.target.files[0])return; bmp=await createImageBitmap(e.target.files[0]); render(); };
document.querySelectorAll('input').forEach(i=>{ if(i.id!=='file') i.oninput=render; });

function render(){
  if(!bmp||!W)return;
  const s=Math.max(W/bmp.width,H/bmp.height), dw=bmp.width*s, dh=bmp.height*s;
  ctx.drawImage(bmp,(W-dw)/2,(H-dh)/2,dw,dh);
  const img=ctx.getImageData(0,0,W,H), d=img.data, n=W*H;
  if(kind()==='pic'){
    out=new Uint8Array(n*2);
    for(let i=0;i<n;i++){const v=((d[i*4]&0xF8)<<8)|((d[i*4+1]&0xFC)<<3)|(d[i*4+2]>>3); out[i*2]=v&255; out[i*2+1]=v>>8;}
    return;
  }
  // Line art: ink = noticeably darker than the local average (copes with uneven lighting).
  const g=new Float32Array(n); for(let i=0;i<n;i++) g[i]=0.299*d[i*4]+0.587*d[i*4+1]+0.114*d[i*4+2];
  const S=new Float64Array((W+1)*(H+1));
  for(let y=0;y<H;y++){let r=0;for(let x=0;x<W;x++){r+=g[y*W+x];S[(y+1)*(W+1)+x+1]=S[y*(W+1)+x+1]+r;}}
  const k=Math.max(8,Math.round(W/16)), sens=+$('sens').value/100, inv=$('inv').checked;
  out=new Uint8Array((n+7)>>3);
  for(let y=0;y<H;y++)for(let x=0;x<W;x++){
    const x0=Math.max(0,x-k),x1=Math.min(W,x+k+1),y0=Math.max(0,y-k),y1=Math.min(H,y+k+1);
    const m=(S[y1*(W+1)+x1]-S[y0*(W+1)+x1]-S[y1*(W+1)+x0]+S[y0*(W+1)+x0])/((x1-x0)*(y1-y0));
    const i=y*W+x, ink=inv? g[i]>m*(1+sens) : g[i]<m*(1-sens);
    if(ink) out[i>>3]|=0x80>>(i&7);
    const c=ink?0:255; d[i*4]=d[i*4+1]=d[i*4+2]=c;
  }
  ctx.putImageData(img,0,0);
}
$('send').onclick=async()=>{
  if(!out){say('Choose a photo first');return;}
  const fd=new FormData(); fd.append('f',new Blob([out]),'data.bin');
  say('Sending...');
  const r=await fetch(kind()==='pic'?'/image':'/lineart',{method:'POST',body:fd});
  say(r.ok?'Done - now showing on the device':'Failed: '+await r.text());
};
document.querySelectorAll('[data-m]').forEach(b=>b.onclick=async()=>{
  const r=await fetch('/mode?m='+b.dataset.m,{method:'POST'}); say(r.ok?'':await r.text());
});
</script></body></html>)HTML";
