parsed = false;

function reqListener() {
  console.log(this.statusText);
  console.log(this.responseText);
}

function send_request(body) {
    var req =  new XMLHttpRequest();
    req.addEventListener("load", reqListener);
    req.open("POST", "http://127.0.0.1:33777");
    req.send(body);
}

function _arrayBufferToBase64( buffer ) {
    var binary = '';
    var bytes = new Uint8Array( buffer );
    var len = bytes.byteLength;
    for (var i = 0; i < len; i++) {
        binary += String.fromCharCode( bytes[ i ] );
    }
    return window.btoa( binary );
}

function create_and_send_command() {
  if(!parsed){
    read_and_parse();
  }
  var body = "{";
  var input = document.getElementById("Send_command").value;
  var input_json = JSON.parse(input);
  for(var k in input_json) {
    if(k == "method") {
      body += '"method": "' + input_json[k] + '", ';
    } else if (k == "Msg") {
      var proto_msg =  new_proto_msg(input_json[k]);
      body +=  '"msg": "' + _arrayBufferToBase64(proto_msg) + '"';
    }
  }
  body += "}";
  var req =  new XMLHttpRequest();
  req.addEventListener("load", reqListener);
  req.open("POST", "http://127.0.0.1:33777");
  console.log("Sending: " + body);
  req.send(body);
}

function new_proto_msg(json_data) {
  if(!parsed){
    read_and_parse();
  }
  for(var k in json_data) {
    var msg = root.lookupType(k);
    var payload = json_data[k];
    var err = msg.verify(payload);
    if (err) {
      console.log("error when creating msg: " + err);
      return;
    }
    var message = msg.create(payload);
    var buffer = msg.encode(message).finish();
  }
  return buffer;
}

function create_msg() {
  if(!parsed){
    read_and_parse();
  }
  try {
    var input_str = document.getElementById("create_msg").value;
    var input_json = JSON.parse(input_str);
    for(var k in input_json) {
      var msg = root.lookupType(k);
      var payload = input_json[k];
      var err = msg.verify(payload);
      if (err) {
        document.getElementById("create_msg").value = err;
        return;
      }
      //Create
      var message = msg.create(payload);
      //Encode
      var buffer = msg.encode(message).finish();
      //Log and send
      console.log(buffer);
      send_request(buffer);
    }
  }
  catch(err) {
    document.getElementById("create_msg").value = err;
  }
}

function read_and_parse() {
  if(parsed){
    return;
  }
  try {
    proto_contents = document.getElementById(".proto").value;
    parsed_proto = protobuf.parse(proto_contents, { keepCase: true })
    root = parsed_proto.root
    parsed = true;
  }
  catch(err) {
    document.getElementById(".proto").value = err;
  }
}

function buffer_to_json(buffer) {
  if(!parsed){
    read_and_parse();
  }
  document.getElementById("buffer_to_json").value = root.toJSON(buffer);
}

function base64_encode() {
  var input = document.getElementById("base64_encoder").value;
  var result = window.btoa(input);
  console.log(result);
  document.getElementById("base64_encoder").value = result;
}
