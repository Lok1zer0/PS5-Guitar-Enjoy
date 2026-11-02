// SPDX-License-Identifier: GPL-3.0-or-later
// Frontend test only: synthetic status server, never connects to a PS5.
const {chromium}=require(process.argv[2]||'playwright');
const http=require('node:http');
const fs=require('node:fs');
const assert=require('node:assert/strict');
(async()=>{
 let browser,server,pending,delay=false,stopRequests=0;
 let state={connected:true,has_report:false,node:'0.4',endpoint:129,packet_size:32,remaining_s:150,
   reports:50,invalid:0,changes:20,age_ms:0,max_gap_us:10000,buttons:31,whammy:200,
   slider:21,accel:[624,512,512],presses:[1,1,1,1,1,0,0,0,0,0],nodes:2,errors:0,errno:0,raw:'STALE'};
 const html=fs.readFileSync('web/index.html','utf8');
 const json=res=>{res.writeHead(200,{'Content-Type':'application/json','Cache-Control':'no-store'});res.end(JSON.stringify(state));};
 try{
  server=http.createServer((req,res)=>{
   if(req.url.endsWith('/state')){if(delay){pending=res;return;}json(res);return;}
   if(req.url.endsWith('/stop')){stopRequests++;res.writeHead(200,{'Content-Type':'application/json'});res.end('{"stopped":true}');return;}
   res.writeHead(200,{'Content-Type':'text/html; charset=utf-8'});res.end(html);
  });
  await new Promise(r=>server.listen(0,'127.0.0.1',r));
  browser=await chromium.launch({channel:'msedge',headless:true});
  const page=await browser.newPage({viewport:{width:1100,height:1100}});
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto(`http://127.0.0.1:${server.address().port}/0123456789abcdef/`);
  await page.waitForFunction(()=>document.querySelector('#status').textContent!=='Conectando ao diagnóstico…');
  assert.match(await page.locator('#status').innerText(),/aguardando relatórios/,'reconnect must wait for current-session input');
  assert.equal(await page.locator('[data-bit].on').count(),0,'historical buttons must stay dark');
  assert.doesNotMatch(await page.locator('#raw').innerText(),/STALE/,'historical raw report must not label new connection');
  state={...state,has_report:true,buttons:3,raw:'06 00 0F 80 80 C8 15 …'};
  await page.waitForFunction(()=>document.querySelectorAll('.fret.on').length===2);
  assert.equal(await page.locator('#whammy').innerText(),'200');
  await page.screenshot({path:'build/interface-preview.png',fullPage:true});
  delay=true;
  for(let i=0;i<100&&!pending;i++)await new Promise(r=>setTimeout(r,20));
  assert.ok(pending,'test must hold an actual in-flight state request');
  await page.locator('#stop').click();
  await page.waitForFunction(()=>document.querySelector('#status').textContent==='Diagnóstico encerrado');
  state={...state,buttons:31};json(pending);pending=null;
  await page.waitForTimeout(250);
  assert.equal(await page.locator('#status').innerText(),'Diagnóstico encerrado','late state response must not overwrite stopped screen');
  assert.equal(await page.locator('[data-bit].on').count(),0);
  assert.equal(stopRequests,1);assert.deepEqual(errors,[]);
  console.log('UI: session freshness, chord display, stop race PASS (synthetic server, Edge)');
 } finally{
  if(pending)pending.end();
  if(browser)await browser.close();
  if(server){server.closeAllConnections();await new Promise(r=>server.close(r));}
 }
})().catch(e=>{console.error(e);process.exitCode=1;});
