var parser = require('./parser.js');
var Clay = require('pebble-clay');
var messageKeys = require('message_keys');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

const WATCARD_URL_BEGIN = 'http://watcard.uwaterloo.ca/oneweb/watgopher661.asp?';
const WATCARD_URL_END = '&FINDATAREP=ON&MESSAGEREP=ON&STATUS=STATUS&watgopher_title=WatCard+Account+Status';
const KEY_ACCOUNT_ID = 'account_id';
const KEY_PIN = 'account_pin';

function sendAppMessage(data) {
  console.log('Sending data: ' + JSON.stringify(data));
  Pebble.sendAppMessage(data, function(e) {
    console.log('Sent data to Pebble');
  }, function(e) {
    console.log('Failed to send data!');
    console.log(JSON.stringify(e));
  });
}

// Send a generic error message back
function sendError() {
  messageDict = {};
  messageDict[messageKeys.Error] = 1;
  sendAppMessage(messageDict);
}

function getWatCardHTML(account_id, pin) {
  var req = new XMLHttpRequest();
  var url = WATCARD_URL_BEGIN + 'acnt_1=' + account_id + '&acnt_2=' + pin + WATCARD_URL_END;
  console.log('url ' + url);
  req.open('POST', url, true);
  req.onload = function(e) {
    if (req.readyState == 4 && req.status == 200) {
      var response = req.responseText;
      parser.parse(response, function onResult(balance){
        messageDict = {};
        messageDict[messageKeys.MealBalance] = Math.floor(balance.meal);
        messageDict[messageKeys.FlexBalance] = Math.floor(balance.flex);
        sendAppMessage(messageDict);
      }, function onError(){
        console.log('Error on parse!');
        sendError();
      });
    } else {
      console.log('Error on fetch. New URL perhaps?');
      sendError();
    }
  }
  req.send(null);
}

function refresh() {
  var account_id = localStorage[KEY_ACCOUNT_ID];
  messageDict = {};
  if (account_id == '1234') {
    messageDict[messageKeys.MealBalance] = 367;
    messageDict[messageKeys.FlexBalance] = 195;
    sendAppMessage(messageDict);
  } else {
    getWatCardHTML(account_id, localStorage[KEY_PIN]);
  }
}

Pebble.addEventListener("ready",
    function ready(e) {
      console.log("JS Ready");
      messageDict = {};
      messageDict[messageKeys.JSReady] = 1;
      sendAppMessage(messageDict);
    }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
    if (e.payload.RequestRefresh) {
      refresh();
    }
  }
);

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }

  var dict = clay.getSettings(e.response);
  localStorage[KEY_ACCOUNT_ID] = dict[messageKeys.Dummy_AccountId];
  localStorage[KEY_PIN] = dict[messageKeys.Dummy_AccountPin];

  refresh();
});
