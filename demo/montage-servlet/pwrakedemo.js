var num = 1;
var timerID;

function createHttpRequest()
{
  if (window.ActiveXObject) {
    try {
      return new ActiveXObject("Msxml2.XMLHTTP")
    } catch (e) {
      try {
        return new ActiveXObject("Microsoft.XMLHTTP");
      } catch (e2) {
        return null
      }
    }
  } else if(window.XMLHttpRequest){
    return new XMLHttpRequest();
  } else {
    return null
  }
}


function requestFile( data, method, fileName, async, func )
{
  var httpoj = createHttpRequest();
  httpoj.open( method , fileName , async );
  httpoj.onreadystatechange = function() {
    if (httpoj.readyState==4) {
      func(httpoj.responseText);
    }
  }
  httpoj.send( data );
}


function showMessage(message)
{
  var span = document.getElementById('message');
  span.innerHTML = message;

  if ( message == 'workflow started' ) {
    eraseImages();
    timerID = setTimeout('progress()',2);
  }
}


function eraseImages() {
  var attach = parent.fr2.document.getElementById('attach');
  while (attach.firstChild) {
    attach.removeChild(attach.firstChild);
  }
  num = 1;
}


function progressProc(input)
{
  var list = input.split(/\n/);

  for (var i in list) {
    var line = list[i];

    if (line.length > 0) {

      if (line == 'reload') {
        loadSvg();

      } else if (line.substring(0,6) == 'start ') {
	var res = line.substring(6);
	changeEllipseAttr(res,'doing');

      } else if (line.substring(0,4) == 'end ') {
	var res = line.substring(4);
	changeEllipseAttr(res,'done');
	changePolygonAttr(res,'done');

      } else {
	var res = line;
	var img = null;
	if (line.substring(0,4) == 'img '){
	  img = new Image();
	  res = line.substring(4);
	  img.src = res.replace(/\.fits?$/,'.jpg');
	}
	var href = "";
	if (res.match(/fits?$/)) {
	  href = res;
	} else {
	  href = "Javascript:w=window.open('"+res+"','','scrollbars=yes,width=640,height=640,');w.focus();";
	}
	var a = parent.fr2.document.createElement("a");
	a.setAttribute('href',href);
	var txt = parent.fr2.document.createTextNode(res);
	a.appendChild(txt);
	var text = parent.fr2.document.createTextNode(""+(num++)+": ");
	var div = parent.fr2.document.createElement("div");
	div.appendChild(text);
	div.appendChild(a);
	if (img) {
	  div.appendChild(img);
	}
	var attach = parent.fr2.document.getElementById('attach');
	attach.appendChild(div);
	scrollToBottom();

	// workflow finish
	if (line == "finish") {
	  showMessage("workflow finished");
	  return;
	}
      }
    }
  }
  timerID = setTimeout('progress()',1);
}


function scrollToBottom() {
  // scroll to bottom
  var height = parent.fr2.document.documentElement.scrollHeight || parent.fr2.document.body.scrollHeight;
  parent.fr2.scrollTo(0,height);
}


function changePolygonAttr(tag,attr) {
  var elem = parent.fr3.document.getElementById(tag);
  if (elem) {
    var elip = elem.getElementsByTagName('polygon')[0];
    if (elip) {
      elip.setAttribute('class',attr);
    }
  }
}


function changeEllipseAttr(tag,attr) {
  var elem = parent.fr3.document.getElementById(tag);
  if (elem) {
    var elip = elem.getElementsByTagName('ellipse')[0];
    if (elip) {
      elip.setAttribute('class',attr);
    }
  }
}


function loadSvg()
{
  var doc = parent.fr3.document;
  var elm = doc.getElementById("workflowgraph");
  while (elm.firstChild) {
    elm.removeChild(elm.firstChild);
  }

  var http = createHttpRequest();
  http.open("GET", "/graph.svg", false );
  http.send(null);
  if (http.readyState == 4 && http.status == 200) {
    var xml = http.responseXML;
    var svgnodes = xml.getElementsByTagName("svg");
    var svg = document.importNode(svgnodes[0], true);
    //svg.setAttributeNS(NS_URI, "preserveAspectRatio", "meet");
    var sel = document.testform.graphsize;
    var mag = parseFloat(sel.options[sel.selectedIndex].value);
    var wid = doc.defaultView.getComputedStyle(doc.documentElement, null).getPropertyValue('width');
    svg.style.width = (mag*0.01*parseInt(wid)).toString(10)+"px";
    elm.appendChild(svg);
  }
}


function nodeIn(e) {
  var pre = '';
  var p = e.target.parentNode;
  var poly = p.getElementsByTagName('polygon')[0];
  if (poly) {poly.setAttribute('style','fill: #ff4400;');pre='file: '}
  var elip = p.getElementsByTagName('ellipse')[0];
  if (elip) {elip.setAttribute('style','fill: #ff4400;');pre='task: '}
  var text = p.getElementsByTagName('text')[0].textContent;
  var elem = parent.fr1.document.getElementById('node_note');
  elem.textContent = pre+text;
}


function nodeOut(e) {
  var p = e.target.parentNode;
  var poly = p.getElementsByTagName('polygon')[0];
  if (poly) {poly.removeAttribute('style');}
  var elip = p.getElementsByTagName('ellipse')[0];
  if (elip) {elip.removeAttribute('style');}
  var elem = parent.fr1.document.getElementById('node_note');
  elem.textContent = "";
}


function graphSize() {
  var doc = parent.fr3.document;
  var elm = doc.getElementById("workflowgraph");
  var svg = elm.getElementsByTagName("svg")[0];

  var sel = document.testform.graphsize;
  var mag = parseFloat(sel.options[sel.selectedIndex].value);
  var wid = doc.defaultView.getComputedStyle(doc.documentElement, null).getPropertyValue('width');
  svg.style.width = (mag*0.01*parseInt(wid)).toString(10)+"px";
}


function startWorkflow() {
  var sel = document.testform.selnodes;
  var val = sel.options[sel.selectedIndex].value;
  requestFile( val, 'POST', 'start', true, showMessage );
}


function stopWorkflow() {
  requestFile( '', 'POST', 'stop', true, showMessage );
  clearTimeout(timerID);
}


function progress() {
  requestFile( '', 'GET', 'progress', true, progressProc );
}

