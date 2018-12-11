-- connections_graph.lua

function create_graph_html(graph)
  local html = [[
<html>
<head>

<script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/vis/4.21.0/vis.min.js"></script>
<link href="https://cdnjs.cloudflare.com/ajax/libs/vis/4.21.0/vis.min.css" rel="stylesheet" type="text/css" />

<style type="text/css">
  #mynetwork {
    width: 1000px;
    height: 600px;
    border: 1px solid lightgray;
  }

  pre {
    border: 1px solid black;
    padding: 8px;
    overflow:auto;
    font-size: 16px;
    font-family: 'Inconsolata', monospace;
  }

</style>
</head>
<body>
<br/>
<hr />
<div id="mynetwork"></div>

<script type="text/javascript">
  // create an array with nodes
  var nodes = new vis.DataSet([
  ]]
  ..
    table.concat(graph["nodes"], ",\n")

  ..
  [[
    ]);
    // create an array with edges
    var edges = new vis.DataSet([
  ]]
  ..
    table.concat(graph["edges"], ",\n")
  ..
  [[
    ]);

    // create a network
  var container = document.getElementById('mynetwork');
  var data = {
    nodes: nodes,
    edges: edges
  };
  var options = {
    edges: {
      smooth: {
        type: 'cubicBezier',
        forceDirection: 'horizontal',
        roundness: 0.4
      }
    },
    physics: {
      stabilization: {
        iterations: 500
      }
    }
  };
  var network = new vis.Network(container, data, options);
</script>
</body></html>
  ]]
  return html
end
