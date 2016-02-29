var parser = require('parser');
var Clay = require('clay');
var clayConfig = require('config.json');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

const WATCARD_URL_BEGIN = 'https://account.watcard.uwaterloo.ca/watgopher661.asp?';
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

function getWatCardHTML(account_id, pin) {
  var req = new XMLHttpRequest();
  var url = WATCARD_URL_BEGIN + 'acnt_1=' + account_id + '&acnt_2=' + pin + WATCARD_URL_END;
  req.open('POST', url, true);
  req.onload = function(e) {
    if (req.readyState == 4 && req.status == 200) {
      if(req.status == 200) {
        var response = req.responseText;
        parser.parse(response, function onResult(balance){
          var data = {
            'meal_amount': Math.floor(balance.meal),
            'flex_amount': Math.floor(balance.flex)
          };
          sendAppMessage(data);
        }, function onError(){
          console.log('Error on parse!');
          var data = {
            'error': 1
          };
          sendAppMessage(data);
        });
      }
    } else {
      console.log('Error');
    }
  }
  req.send(null);
}

function refresh() {
  getWatCardHTML(localStorage[KEY_ACCOUNT_ID], localStorage[KEY_PIN]);
}

Pebble.addEventListener("ready",
    function ready(e) {
        console.log("Hello world! - Sent from your javascript application.");
        refresh();
    }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
    if (e.payload.request_refresh) {
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
  localStorage[KEY_ACCOUNT_ID] = dict[KEY_ACCOUNT_ID];
  localStorage[KEY_PIN] = dict[KEY_PIN];

  refresh();
});
