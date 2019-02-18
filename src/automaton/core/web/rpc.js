var PORT = "33777";
var rpc_proto =
`syntax = "proto3";

// --- protocol messages ---

message Protocol {
  bytes protocol_id = 1;
  repeated bytes file_names = 2;
  repeated bytes files = 3;
}

message ProtocolIDsList {
  repeated bytes protocol_ids = 1;
}

message ProtocolsList {
  repeated Protocol protocols = 1;
}

// -- node messages --

message Node {
  bytes id = 1;
  bytes protocol_id = 2;
  bytes address = 3;
}

message NodeID {
  bytes node_id = 1;
}

message NodeIdsList {
  repeated bytes node_ids = 1;
}

message NodesList {
  repeated Node nodes = 1;
}

message NodeCmdRequest {
  bytes node_id = 1;
  bytes cmd = 2;
  bytes params = 3;
}

message NodeCmdResponse {
  bytes response = 1;
}

message PeerIdsList {
  bytes node_id = 1;
  repeated uint32 peer_ids = 2;
}

message PeerAddressesList {
  bytes node_id = 1;
  repeated bytes peer_addresses = 2;
}

message PeersList {
  bytes node_id = 1;
  repeated Peer peers = 2;
}

message Peer {
  uint32 id = 1;
  bytes address = 2;
}

message Test {
  uint32 i = 1;
}

message Test2 {
  uint32 a = 1;
  Test b = 2;
}`;

var protocols;
var nodes;
var files = new Map();

var parsed = false;
var root;

function string_to_uint8array(str) {
  var array = new Uint8Array(str.length);
  for(var i = 0; i < str.length; i++) {
    array[i] = str.charCodeAt(i);
  }
  return array;
}

function array_to_base64(buffer) {
  var binary = '';
  var bytes = new Uint8Array(buffer);
  var len = bytes.byteLength;
  for (var i = 0; i < len; i++) {
      binary += String.fromCharCode(bytes[ i ]);
  }
  return window.btoa(binary);
}

function result_listener() {
  var decoded = window.atob(this.response);
  document.getElementById("status").innerHTML = this.statusText;
  document.getElementById("result").innerHTML = decoded;
}

function protocols_listener() {
  var decoded = window.atob(this.response);
  document.getElementById("status").innerHTML = this.statusText;
  document.getElementById("result").innerHTML = decoded;
  var msg_type = root.lookupType("ProtocolsList");
  var array = string_to_uint8array(decoded);
  protocols = msg_type.decode(array);
  load_protos();
}

function nodes_listener() {
  var decoded = window.atob(this.response);
  document.getElementById("status").innerHTML = this.statusText;
  document.getElementById("result").innerHTML = decoded;
  var msg_type = root.lookupType("NodeIdsList");
  var array = string_to_uint8array(decoded);
  nodes = msg_type.decode(array);
  load_nodes();
}

function create_and_send_command_from_string(input) {
  if(!parsed){
    read_and_parse();
  }
  var input_json = JSON.parse(input);
  var body = "{";
  var method = "";
  for(var k in input_json) {
    if(k == "method") {
      method = input_json[k];
      body += '"method": "' + method + '", ';
    } else if (k == "Msg") {
      var proto_msg =  new_proto_msg(input_json[k]);
      body +=  '"msg": "' + array_to_base64(proto_msg) + '"';
    }
  }
  body += "}";
  var req =  new XMLHttpRequest();
  if (method === "get_protocols") {
    req.addEventListener("load", protocols_listener);
  } else if (method === "list_nodes") {
    req.addEventListener("load", nodes_listener);
  } else {
    req.addEventListener("load", result_listener);
  }
  req.open("POST", "http://127.0.0.1:" + PORT);
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
    // WHAT THE HELL?!?!
    var buffer = msg.encode(message).finish();
  }
  return buffer;
}

function read_and_parse() {
  if(parsed){
    return;
  }
  try {
    parsed_proto = protobuf.parse(rpc_proto, { keepCase: true })
    root = parsed_proto.root
    parsed = true;
  }
  catch(err) {
    document.getElementById("result").innerHTML = err;
  }
}

function base64_encode() {
  var input = document.getElementById("base64_encoder").value;
  var result = window.btoa(input);
  document.getElementById("base64_encoder").value = result;
}

function load_protos() {
  if (protocols == null) {
    console.log("Error: no protocols!");
    return;
  }
  var proto_list = document.getElementById("selected_protocol");
  var file_list = document.getElementById("selected_file");
  for(var i = proto_list.options.length - 1 ; i > 0 ; i--) {
    proto_list.remove(i);
  }
  for(var i = file_list.options.length - 1 ; i > 0 ; i--) {
    file_list.remove(i);
  }
  for (var i = 0; i < protocols.protocols.length; i++) {
    var p = protocols.protocols[i];
    var pid = new TextDecoder("utf-8").decode(p.protocol_id);
    var opt = document.createElement('option');
    opt.value = pid;
    opt.innerHTML = pid;
    proto_list.appendChild(opt);
    console.log("Adding option " + pid);

    for (var j = 0; j < p.file_names.length; j++) {
      var file_name = pid + new TextDecoder("utf-8").decode(p.file_names[j]);
      var file = new TextDecoder("utf-8").decode(p.files[j]);
      files.set(file_name, file);

      var o = document.createElement('option');
      o.value = file_name;
      o.innerHTML = file_name;
      file_list.appendChild(o);
    }
  }
}

function load_nodes() {
  if (nodes == null) {
    console.log("Error: no nodes!");
    return;
  }
  var nodes_list = document.getElementById("selected_node");
  for(var i = nodes_list.options.length - 1 ; i > 0 ; i--) {
    nodes_list.remove(i);
  }
  for (var i = 0; i < nodes.node_ids.length; i++) {
    var n = nodes.node_ids[i];
    var nid = new TextDecoder("utf-8").decode(n);
    var opt = document.createElement('option');
    opt.value = nid;
    opt.innerHTML = nid;
    nodes_list.appendChild(opt);
  }
}

function show_file() {
  var selected_file = document.getElementById("selected_file").value;
  console.log(selected_file);
  if (selected_file === "none") {
    return;
  }
  else {
    if (files.has(selected_file)) {
      document.getElementById("file").innerHTML = files.get(selected_file)
    } else {
      console.log("No such file");
    }
  }

}

// ======================================

function list_supported_protocols() {
  var msg = `{
  "method" : "list_supported_protocols",
  "Msg" :  ""
  }`;
  create_and_send_command_from_string(msg);
}

function get_protocols() {
  document.getElementById("send_command_text_field").value =
  `{
  "method" : "get_protocols",
  "Msg" :  {
    "ProtocolIDsList" : {
      "protocol_ids" : ["YXV0b21hdG9uL2V4YW1wbGVzL3NtYXJ0cHJvdG8vY2hhdC8="]
    }
  }
  }`;
}

function load_protocols() {
  document.getElementById("send_command_text_field").value =
  `{
  "method" : "load_protocols",
  "Msg" :  {
    "ProtocolIDsList" : {
      protocol_ids : []
    }
  }
  }`;
}

function launch_node() {
  var protocol = document.getElementById("selected_protocol").value;
  if (protocol === "none") {
    document.getElementById("send_command_text_field").value = "ERROR: Select protocol first!";
  } else {
    var msg = `{
    "method" : "launch_node",
      "Msg" :  {
        "Node" : {
          "id" : "",
          "protocol_id" : "`
          + window.btoa(protocol) +
          `",
          "address" : ""
        }
      }
    }`;
    document.getElementById("send_command_text_field").value = msg;
  }
}

function list_nodes() {
  var msg = `{
  "method" : "list_nodes",
  "Msg" :  ""
  }`;
  create_and_send_command_from_string(msg);
}

function get_nodes() {
  var msg = `{
  "method" : "get_nodes",
    "Msg" : {
      "NodeIdsList" : {
        "node_ids" : []
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function process_cmd() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "process_cmd",
    "Msg" : {
      "NodeCmdRequest" : {
        "node_id" : "`
        + window.btoa(node) +
        `",
        "cmd" : "",
        "params" : ""
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function add_peers() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "add_peers",
    "Msg" : {
      "PeerAddressesList" : {
        "node_id" : "`
        + window.btoa(node) +
        `",
        "peer_addresses" : []
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function remove_peers() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "remove_peers",
    "Msg" :  {
      "PeerIdsList" : {
        "node_id" : "`
        + window.btoa(node) +
        `",
        "peer_ids" : []
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function list_known_peers() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "list_known_peers",
    "Msg" :  {
      "NodeID" : {
        "node_id" : "`
        + window.btoa(node) +
        `"
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function list_connected_peers() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "list_connected_peers",
    "Msg" : {
      "NodeID" : {
        "node_id" : "`
        + window.btoa(node) +
        `"
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function get_peers() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "get_peers",
    "Msg" : {
      "PeerIdsList" : {
        "node_id" : "`
        + window.btoa(node) +
        `",
        "peer_ids" : []
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function connect() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "connect",
    "Msg" :  {
      "PeerIdsList" : {
        "node_id" : "`
        + window.btoa(node) +
        `",
        "peer_ids" : []
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function disconnect() {
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "disconnect",
    "Msg" :  {
      "PeerIdsList" : {
        "node_id" : "`
        + window.btoa(node) +
        `",
        "peer_ids" : []
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}
