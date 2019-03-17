var PORT = "33777";
var rpc_proto =
`syntax = "proto3";

// *** Smart Protocols ***

message Protocol {
  bytes protocol_id = 1;
  bytes path = 2;
  repeated bytes file_names = 3;
  repeated bytes files = 4;
}

message ProtocolIDsList {
  repeated bytes protocol_ids = 1;
}

message ProtocolsList {
  repeated Protocol protocols = 1;
}

// *** node ***

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

// *** testnet ***

message TestNetNodeConnections {
  // The index of the client node within the testnet, starting at 1.
  uint32 from_node = 1;

  // The index of the listener node within the testnet, starting at 1.
  repeated uint32 to_node = 2;
}

message TestNetCreate {
  // Test network ID. Only one testnet can be created with the same ID.
  bytes testnet_id = 1;

  // The ID of the registered smart protocol we want to launch a testnet with.
  bytes protocol_id = 2;

  // Currently supported: "localhost" or "simulation".
  bytes network_type = 3;

  // Number of peer nodes to instantiate.
  uint32 number_nodes = 4;

  // Initial network topology using node index, starting at 0.
  repeated TestNetNodeConnections topology = 5;
}

message TestNetGetNodeID {
  bytes testnet_id = 1;
  uint32 node_index = 2;
}

message TestNetID {
  bytes testnet_id = 1;
}

// -- network messages --

message Network {
  bytes protocol_id = 1;
  uint32 number_nodes = 2;
  uint32 number_peers_per_node = 3;
  bool is_localhost = 4;  // true -> tcp, false -> sim
  bytes logging_path = 5;
}`;

// TODO(kari): Works for only one .proto file per protocol -> FIX THAT
var loaded_protocols = new Map();

var files = new Map();
files.set("rpc.proto", rpc_proto);

function get_message_template(protocol_id, type) {
  // check if already cashed
  var protocol = loaded_protocols.get(protocol_id);
  var templates = protocol.message_templates;
  var root = protocol.root;
  if (templates.has(type)) {
    return templates.get(type);
  }
  var template = "{\n";
  for(var f in root.nested[type].fields) {
    var ftype = root.nested[type].fields[f].type;
    if (root.nested[type].fields[f].repeated) {  // is repeated
      if (ftype === "string") {
        template += "\t\"" + f + "\" : [\"\"],\n";
      } else if (root.nested.hasOwnProperty(ftype)) {  // is message
        template += "\t\"" + f + "\" : [" + get_message_template(protocol_id, ftype) + "],\n";
      } else {  // is number
        template += "\t\"" + f + "\" : [0],\n";
      }
    } else {
      if (ftype === "string") {
        template += "\t\"" + f + "\" : \"\",\n";
      } else if (root.nested.hasOwnProperty(ftype)) {  // is message
        template += "\t\"" + f + "\" : " + get_message_template(protocol_id, ftype) + ",\n";
      } else {  // is number
        template += "\t\"" + f + "\" : 0,\n";
      }
    }
  }
  template = template.substring(0, template.length - 2) + "\n}";
  protocol.message_templates.set(type, template);

  return protocol.message_templates.get(type);
}

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

function result_handler(protocol_id, cmd) {
  document.getElementById("status").innerHTML = this.statusText;
  var protocol = loaded_protocols.get("rpc");
  var root = protocol.root;
  var decoded = window.atob(this.response);
  var array = string_to_uint8array(decoded);
  var response = "";
  if (protocol_id !== "rpc") {
    try {
      var msg_type = root.lookupType("NodeCmdResponse");
      var message = msg_type.decode(array);

      protocol = loaded_protocols.get(protocol_id);
      root = protocol.root;
      array = message.response;

      if(protocol.commands.get(cmd)[1] !== "") {
        msg_type = root.lookupType(protocol.commands.get(cmd)[1]);
        message = msg_type.decode(array);
        response = JSON.stringify(message.toJSON());
      }
    } catch(err) {
      response = err;
    }
  }
  try {
    if(protocol.commands.get(cmd)[1] !== "") {
      var msg_type = root.lookupType(protocol.commands.get(cmd)[1]);
      var message = msg_type.decode(array);
      response = JSON.stringify(message.toJSON());
    }
  } catch(err) {
    response = err;
  }
  document.getElementById("result").innerHTML = response;
}

function protocols_handler() {
  var root = loaded_protocols.get("rpc").root;
  var decoded = window.atob(this.response);
  document.getElementById("status").innerHTML = this.statusText;
  document.getElementById("result").innerHTML = decoded;
  var msg_type = root.lookupType("ProtocolsList");
  var array = string_to_uint8array(decoded);
  load_protos(msg_type.decode(array));
}

function nodes_handler() {
  var root = loaded_protocols.get("rpc").root;
  var decoded = window.atob(this.response);
  document.getElementById("status").innerHTML = this.statusText;
  document.getElementById("result").innerHTML = decoded;
  var msg_type = root.lookupType("NodeIdsList");
  var array = string_to_uint8array(decoded);
  load_nodes(msg_type.decode(array));
}

function create_and_send_command_from_string(input) {
  var input_json = JSON.parse(input);
  var body = "{";
  var method = "";
  for(var k in input_json) {
    if(k == "method") {
      method = input_json[k];
      body += '"method": "' + method + '", ';
    } else if (k == "Msg") {
      var proto_msg = new_proto_msg("rpc", input_json[k]);
      body += '"msg": "' + array_to_base64(proto_msg) + '"';
    }
  }
  body += "}";
  if (method === "process_cmd") {
    var protocol = document.getElementById("selected_protocol").value;
    var cmd = document.getElementById("selected_cmd").value;
  } else {
    var protocol = "rpc";
    var cmd = method;
  }
  var req = new XMLHttpRequest();
  if (protocol === "rpc" && method === "get_protocols") {
    req.addEventListener("load", protocols_handler);
  } else if (protocol === "rpc" && method === "list_nodes") {
    req.addEventListener("load", nodes_handler);
  } else {
    req.addEventListener("load", result_handler.bind(req, protocol, cmd));
  }
  req.open("POST", "http://127.0.0.1:" + PORT);
  console.log("Sending: " + body);
  req.send(body);
}

function new_proto_msg(protocol, json_data) {
  if (!loaded_protocols.has(protocol)) {
    return;
  }
  var root = loaded_protocols.get(protocol).root;
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

function read_and_parse() {
  if(loaded_protocols.has("rpc")){
    return;
  }
  try {
    // extracting message types and creating templates for each one of them
    var root = (protobuf.parse(rpc_proto, { keepCase: true })).root;

    var cmds = [
      ["list_supported_protocols", ["", "ProtocolIDsList"]],
      ["list_running_protocols", ["", "ProtocolIDsList"]],
      ["get_protocols", ["ProtocolIDsList", "ProtocolsList"]],
      ["load_protocols", ["ProtocolsList", ""]],
      ["launch_node", ["Node", ""]],
      ["list_nodes", ["", "NodeIdsList"]],
      ["get_nodes", ["NodesIdsList", "NodesList"]],
      ["add_peers", ["PeerAddressesList", "PeersList"]],
      ["remove_peers", ["PeerIdsList", ""]],
      ["list_known_peers", ["NodeID", "PeerIdsList"]],
      ["list_connected_peers", ["NodeID", "PeerIdsList"]],
      ["get_peers", ["PeerIdsList", "PeersList"]],
      ["connect", ["PeerIdsList", ""]],
      ["disconnect", ["PeerIdsList", ""]],
      ["process_cmd", ["NodeCmdRequest", "NodeCmdResponse"]],
      ["start_testnet", ["Network", ""]],
      ["testnet_create", ["TestNetCreate", ""]],
      ["testnet_destroy", ["TestNetID", ""]],
      ["testnet_get_node_id", ["TestNetGetNodeID", "NodeID"]]
    ]

    var protocol_struct = {
      name: "rpc",
      root: root,
      proto: rpc_proto,
      commands: new Map(cmds),
      message_templates: new Map()
    }
    loaded_protocols.set("rpc", protocol_struct);
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

function load_protos(protocols) {
  if (protocols == null) {
    console.log("Error: no protocols!");
    return;
  }
  var proto_list = document.getElementById("selected_protocol");
  var file_list = document.getElementById("selected_file");
  for(var i = proto_list.options.length - 1 ; i > 1 ; i--) {
    proto_list.remove(i);
  }
  for(var i = file_list.options.length - 1 ; i > 1 ; i--) {
    file_list.remove(i);
  }
  for (var i = 0; i < protocols.protocols.length; i++) {
    var p = protocols.protocols[i];
    var pid = new TextDecoder("utf-8").decode(p.protocol_id);

    if (p.file_names.length != 2) {
      console.log("Trying to parse " + pid);
      console.log("Protocols with more than 2 files (1 .proto and 1 config.json) are not supported for the moment!");
      continue;
    }

    var opt = document.createElement('option');
    opt.value = pid;
    opt.innerHTML = pid;
    proto_list.appendChild(opt);

    for (var j = 0; j < p.file_names.length; j++) {
      var file_name = pid + '/' + new TextDecoder("utf-8").decode(p.file_names[j]);
      var file = new TextDecoder("utf-8").decode(p.files[j]);
      files.set(file_name, file);
      var o = document.createElement('option');
      o.value = file_name;
      o.innerHTML = file_name;
      file_list.appendChild(o);
      if (file_name === (pid + "/config")) {
        var config_file = file;
      } else {
        var proto_file = file;
      }
    }

    // extracting commands
    var cmds = JSON.parse(config_file).commands;
    var commands = new Map();
    for (var i = 0; i < cmds.length; i++) {
      commands.set(cmds[i][0], [cmds[i][1],cmds[i][2]]);
    }

    // extracting message types and creating templates for each one of them
    var root = (protobuf.parse(proto_file, { keepCase: true })).root;
    var message_templates = new Map();

    var protocol_struct = {
      name: pid,
      root: root,
      proto: proto_file,
      commands: commands,
      message_templates: message_templates
    }

    loaded_protocols.set(pid, protocol_struct);
  }
}

function load_nodes(nodes) {
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
  if (selected_file === "none") {
    document.getElementById("file").innerHTML = "";
    return;
  }
  else {
    if (files.has(selected_file)) {
      document.getElementById("file").innerHTML = files.get(selected_file);
    } else {
      console.log("No such file");
    }
  }

}

function load_commands() {
  var cmd_list = document.getElementById("selected_cmd");
  for(var i = cmd_list.options.length - 1 ; i > 0 ; i--) {
    cmd_list.remove(i);
  }
  var protocol = document.getElementById("selected_protocol").value;
  if (protocol === "none") {
    return;
  }
  if (protocol === "rpc") {
    // TODO(kari): Remove buttons and implement logic here
    return;
  }
  for (var cmd of loaded_protocols.get(protocol).commands.keys()) {
    var opt = document.createElement('option');
    opt.value = cmd;
    opt.innerHTML = cmd;
    cmd_list.appendChild(opt);
  }
}

function show_command() {
  var cmd = document.getElementById("selected_cmd").value;
  if (cmd === "none") {
    return;
  }
  var protocol = document.getElementById("selected_protocol").value;
  if (protocol === "none") {
    return;
  }
  var node = document.getElementById("selected_node").value;
  if (node === "none") {
    node = "";
  }
  var msg = `{
  "method" : "process_cmd",
    "Msg" :  {
      "NodeCmdRequest" : {
        "node_id" : "`
        + window.btoa(node) +
        `",
        "cmd" : "`
        + window.btoa(cmd) +
        `",
        "params" : ""
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
  var cmd_input = loaded_protocols.get(protocol).commands.get(cmd)[0];
  if (cmd_input !== "") {
    document.getElementById("message_encoder").value = "{\n\"" + cmd_input + "\" : " +
        get_message_template(protocol, cmd_input) + "\n}";
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
      "protocol_ids" : [""]
    }
  }
  }`;
}

function load_protocols() {
  document.getElementById("send_command_text_field").value =
  `{
  "method" : "load_protocols",
  "Msg" :  {
    "ProtocolsList" : {
      "protocols" : [
         {"protocol_id": "", "path": ""}
      ]
    }
  }
  }`;
}

function launch_node() {
  var protocol = document.getElementById("selected_protocol").value;
  if (protocol === "none" || protocol === "rpc") {
    return;
  }
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

function create_testnet() {
  var msg = `{
  "method" : "testnet_create",
    "Msg" :  {
      "TestNetCreate" : {
        "testnet_id" : "`
        + window.btoa("testnet") +
        `",
        "protocol_id" : "`
        + window.btoa("chat") +
        `",
        "network_type" : "`
        + window.btoa("localhost") +
        `",
        "number_nodes" : 5,
        "topology" : [
          {"from_node" : 1, "to_node" : [2,3]},
          {"from_node" : 2, "to_node" : [4,5]}
        ]
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function destroy_testnet() {
  var msg = `{
  "method" : "testnet_destroy",
    "Msg" :  {
      "TestNetID" : {
        "testnet_id" : "`
        + window.btoa("testnet") +
        `"
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function destroy_testnet() {
  var msg = `{
  "method" : "testnet_destroy",
    "Msg" :  {
      "TestNetID" : {
        "testnet_id" : "`
        + window.btoa("testnet") +
        `"
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}

function get_testnet_node_id() {
  var msg = `{
  "method" : "testnet_get_node_id",
    "Msg" :  {
      "TestNetGetNodeID" : {
        "testnet_id" : "`
        + window.btoa("testnet") +
        `",
        "node_index" : 1
      }
    }
  }`;
  document.getElementById("send_command_text_field").value = msg;
}
