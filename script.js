const forward = document.getElementById("forward");
const left = document.getElementById("left");
const spray = document.getElementById("spray");
const right = document.getElementById("right");
const backward = document.getElementById("backward");
const btnArray = document.querySelectorAll("button");
const stream = document.getElementById("stream");
const camStatus = document.querySelector(".cam-status");
const speedCtrl = document.querySelector("#speed-ctrl");
const signalStatus = document.getElementById("signal");
const speedStatus = document.getElementById("speed");

let gateway = "ws://main-robot.local/ws";
let websocket;
let isConnected = false;
let lastFrameTime = Date.now();

window.addEventListener("load", onLoad);

const watchdog = setInterval(() => {
  if (Date.now() - lastFrameTime > 5000) {
    setCamStatus(false);
    restartStream();
  }
}, 3000);

function startStream() {
  stream.src = "http://live-stream.local";
}

function setCamStatus(connected) {
  camStatus.textContent = connected
    ? "Camera: Connected"
    : "Camera: Disconnected";

  if (connected) {
    stream.classList.remove("disconnected");
    camStatus.classList.remove("disconnected");
    stream.classList.add("connected");
    camStatus.classList.add("connected");
  } else {
    stream.classList.remove("connected");
    camStatus.classList.remove("connected");
    stream.classList.add("disconnected");
    camStatus.classList.add("disconnected");
  }
}

let restarting = false;

function restartStream() {
  if (restarting) return;
  restarting = true;
  stream.src = "";

  setTimeout(() => {
    stream.src = "http://live-stream.local";
    restarting = false;
  }, 1500);
}

function initWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
  console.log("Connection opened");
  isConnected = true;
  getReadings();
  signalStatus.innerHTML = "Connected";
}

function onClose(event) {
  console.log("Connection closed");
  if (isConnected) {
    clearReadings();

    isConnected = false;
  }
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  const data = JSON.parse(event.data);
  const keys = Object.keys(data);

  keys.forEach((key) => {
    document.getElementById(key).innerHTML = data[key];
  });
}

function onLoad(event) {
  initWebSocket();
  startStream();
  stream.addEventListener("load", () => {
    lastFrameTime = Date.now();
    setCamStatus(true);
  });

  stream.addEventListener("error", () => {
    setCamStatus(false);
    restartStream();
  });
}

function clearReadings() {
  document.getElementById("battery").innerHTML = "...";
  document.getElementById("volume").innerHTML = "...";
  document.getElementById("speed").innerHTML = "...";
  document.getElementById("signal").innerHTML = "...";
  signalStatus.innerHTML = "Disconnected";
}

function getReadings() {
  if (isConnected) websocket.send("getReadings");
}

forward.addEventListener("mousedown", (event) => {
  if (isConnected) websocket.send("forward");
  console.log(event);
});

left.addEventListener("mousedown", () => {
  if (isConnected) websocket.send("left");
});

spray.addEventListener("mousedown", () => {
  if (isConnected) websocket.send("spray");
});

right.addEventListener("mousedown", () => {
  if (isConnected) websocket.send("right");
});

backward.addEventListener("mousedown", () => {
  if (isConnected) websocket.send("backward");
});

speedCtrl.addEventListener("change", () => {
  if (isConnected) websocket.send("speedVal:" + speedCtrl.value);
  speedStatus.innerHTML = speedCtrl.value;
});

btnArray.forEach((item) => {
  if (item.id !== "spray") {
    item.addEventListener("mouseup", (event) => {
      if (isConnected) {
        console.log(event);
        websocket.send("stop");
      }
    });
  }
});

spray.addEventListener("mouseup", () => {
  if (isConnected) websocket.send("stop-spray");
});

setInterval(() => {
  if (websocket.readyState === WebSocket.OPEN) {
    websocket.send("ping");
  }
}, 5000);
