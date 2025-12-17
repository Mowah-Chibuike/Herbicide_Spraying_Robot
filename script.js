const forward = document.getElementById("forward");
const left = document.getElementById("left");
const spray = document.getElementById("spray");
const right = document.getElementById("right");
const backward = document.getElementById("backward");
const btnArray = document.querySelectorAll("button");

console.log(btnArray);

let gateway = "ws://main-robot.local/ws";
let websocket;

window.addEventListener("load", onLoad);

function initWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
  console.log("Connection opened");
  getReadings();
}

function onClose(event) {
  console.log("Connection closed");
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  console.log(event.data);
  const data = JSON.parse(event.data);
  const keys = Object.keys(data);

  keys.forEach((key) => {
    document.getElementById(key).innerHTML = data[key];
    console.log();
  });
}

function onLoad(event) {
  initWebSocket();
}

function getReadings() {
  websocket.send("getReadings");
}

forward.addEventListener("mousedown", (event) => {
  websocket.send("forward");
  console.log(event);
});

left.addEventListener("mousedown", () => {
  websocket.send("left");
});

spray.addEventListener("mousedown", () => {
  websocket.send("spray");
});

right.addEventListener("mousedown", () => {
  websocket.send("right");
});

backward.addEventListener("mousedown", () => {
  websocket.send("backward");
});

btnArray.forEach((item) => {
  item.addEventListener("mouseup", (event) => {
    console.log(event);
    websocket.send("stop");
  });
});

spray.addEventListener("mouseup", () => {
  websocket.send("stop-spray");
});
