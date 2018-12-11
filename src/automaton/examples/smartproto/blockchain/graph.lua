-- graph.lua

-- GRAPH VISUALIZATION OF BLOCK TREE

function hashstr(h)
  return hex(h):sub(-8,-1)
end

function debug_html()
  local n = {}
  local e = {}

  local bb = {}
  for i = 1, #blockchain do
    table.insert(bb, tostring(i) .. ": " .. hex(blockchain[i]))
  end

  -- GENESIS_HASH
  local s
  GH = hashstr(GENESIS_HASH)
  s = string.format("{id: '%s', shape: 'box', label: 'GENESIS', color: '#D2B4DE', level: %d}", GH, 0)
  table.insert(n, s)

  local clr
  for k,v in pairs(blocks) do
    to = hashstr(k)
    from = hashstr(v.prev_hash)
    -- check if this is in current blockchain
    if k == blockchain[v.height] then
      clr = "'#cce0ff', font: {face:'Play'}"
    else
      clr = "'#f2e6d9', font: {color:'#333', face:'Play'}"
    end
    title = "mined by " .. v.miner
        .. "<br>HASH: " .. hex(k)
        .. "<br>HEIGHT: " .. v.height
    s = string.format("{id: '%s', shape: 'box', label: '%s', color: %s, level: %d, title: '%s'}",
      to, to, clr, v.height, title)
    table.insert(n, s)
    s = string.format("{from: '%s', to: '%s', arrows:'to'}", from, to)
    table.insert(e, s)
  end

  collect_balances()
  local balances_html = {}
  for k, v in pairs(balances) do
    -- print(k .. " -> " .. v)
    local s = string.format([[
    <tr>
      <td>%s</td>
      <td>%d</td>
    </tr>
    ]], k, v);
    -- print(s)
    table.insert(balances_html, s)
  end

  local html =
[[
<script type="text/javascript" charset="utf8" src="https://code.jquery.com/jquery-3.3.1.min.js"></script>
<link rel="stylesheet" type="text/css" href="https://cdn.datatables.net/1.10.19/css/jquery.dataTables.css">

<script type="text/javascript" charset="utf8" src="https://cdn.datatables.net/1.10.19/js/jquery.dataTables.js"></script>
<div id="mynetwork"></div>

<script type="text/javascript">
  // create an array with nodes
  var nodes = new vis.DataSet([
]]
..

  table.concat(n, ",\n")
  --[[
    {id: "a1", label: 'Node 1'},
    {id: "a2", label: 'Node 2'},
    {id: "a3", label: 'Node 3'},
    {id: "a4", label: 'Node 4'},
    {id: "a5", label: 'Node 5'},
    {id: "a6", label: 'Node 6'},
    {id: "a7", label: 'Node 7'},
    {id: "a8", label: 'Node 8'}
  ]]

..
[[
  ]);
  // create an array with edges
  var edges = new vis.DataSet([
]]
..

  table.concat(e, ",\n")
--[[
    {from: "a1", to: "a8", arrows:'to', dashes:true},
    {from: "a1", to: "a3", arrows:'to'},
    {from: "a1", to: "a2", arrows:'to, from'},
    {from: "a2", to: "a4", arrows:'to, middle'},
    {from: "a2", to: "a5", arrows:'to, middle, from'},
    {from: "a5", to: "a6", arrows:{to:{scaleFactor:2}}},
    {from: "a6", to: "a7", arrows:{middle:{scaleFactor:0.5},from:true}}

]]

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
    layout: {
      hierarchical: {
        direction: "LR",
        levelSeparation: 120,
        nodeSpacing: 100
      }
    },
    physics: false
  };
  var network = new vis.Network(container, data, options);
  $(document).ready( function () {
    $('#balances').DataTable({
        "scrollY":        "400",
        "scrollCollapse": true,
        "paging":         false,
        "searching":      false,
    });
  } );
</script>
<font size="2" face="Courier New" >
<style>
table {
  text-align: center;
  border: 1px solid #c5cbd6;
}

</style>
<table id="balances" class="display compact">
  <thead>
    <tr>
      <th>Miner</th>
      <th>Number of blocks</th>
    </tr>
  </thead>
  <tbody>
    ]]
    .. table.concat(balances_html, "\n") ..
    [[
  </tbody>
</table>
</font>
]]

  return html;
end
