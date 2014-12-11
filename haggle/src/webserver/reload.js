var req, reqN;

function displayResult() {
	if (req.readyState == 4) {
		if (req.status == 200) {
			document.getElementById("result").innerHTML = req.responseText;
		} else {
			document.getElementById("loading").innerHTML = req.statusText;
		}
	}
}

function displayNeighbor() {
	if (reqN.readyState == 4) {
		if (reqN.status == 200) {	
			document.getElementById("neighbor").innerHTML = reqN.responseText;
		} else {
			document.getElementById("loading").innerHTML = reqN.statusText;
		}
	}
}


function getResult() 
{
	setTimeout("getResult()", 5000);

	req = new XMLHttpRequest();	
	req.onreadystatechange = displayResult;
	req.open("post", "result.html", true);
	req.send("");

	reqN = new XMLHttpRequest();	
	reqN.onreadystatechange = displayNeighbor;
	reqN.open("post", "neighbor.html", true);
	reqN.send("");
}

