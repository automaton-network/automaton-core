-- visualize.lua

function debug_html()
  local html = "<div class='tablewrap'>"
  for i = 1, ROOM_COUNT do
    html = html .. get_room_html(i)
  end
  html = html .. [[
  </div>
  <div id="noteID", class="note">Tooltip text</div>
  <br />
  <style>
  .busy {
  	margin: 8px;
    padding: 4px;
    background: #d5c7d5;
    color: black
  }
  .calendar {
    float: left;
  }
  .tablewrap {
    height: 100%;
  }
  .selected {
    margin: 8px;
    padding: 4px;
    background: #58b7bf;
    color: black
  }
  table {
  	width: 300px;
    height: 300px;
    font-family: Verdana, sans-serif;
    font-size: 18px;
    background: #edf6f6;
    align: left;
    text-align: center;
    border: "0";
    margin: 10px;
    cursor: pointer;
  }
  caption {
    align: "center";
    text-align: center;
    font-size: 18px;
    border: "0";
    margin: 0;
    background: #7dc2c8;
    cursor: default;
  }
  td {
    margin: 8px;
    padding: 4px;
  }
  .note {
    visibility: hidden;
    font-family: Verdana, sans-serif;
    font-size: 18px;
    background: #edf6f6;
    align: right;
    text-align: center;
    border: "0";
    margin: 0;
    position: absolute;
    top: 0;
    left: 500px;
  }
  </style>
  <script>
  var tables = document.getElementsByClassName("calendar");
  for (var k = 0; k < tables.length; k++) {
    var table = tables[k]
  	for (var i = 0; i < table.rows.length; i++) {
          for (var j = 0; j < table.rows[i].cells.length; j++) {
  			table.rows[i].cells[j].onmouseover = function () {
  				on_selection(this, k, true);
  			};
              table.rows[i].cells[j].onmouseleave = function () {
  				on_selection(this, k, false);
  			};
          }
  	}
  }

  function on_selection(table_cell, table_number, over) {
  	var current = document.getElementsByClassName("selected");
      if (current.length > 0) {
        current[0].className = current[0].className.replace(" selected", "");
      }
      var note = document.getElementById("noteID");
      if(over) {
      	table_cell.className += " selected";
          note.innerHTML = table_cell.id;
      	note.style = "visibility:visible";
      }
      else {
      	note.style = "visibility:hidden";
      }
  }
  </script>
  ]]
  return html
end

function get_room_html(room_number)
  local table_html = [[
  <table class="calendar">
    <caption>ROOM ]] .. tostring(room_number) .. [[</caption>
    <tr>
  ]]
  for i = 1, DAYS do
    if (i - 1) % 5 == 0 then
      table_html = table_html .. "</tr><tr>"
    end
    if rooms[room_number][i] then
      table_html = table_html .. "<td class=\"busy\" id='"
        .. validators[rooms[room_number][i]] .. " | "
        .. hex(rooms[room_number][i]) .. "'>" .. tostring(i) .. "</td>"
    else
      table_html = table_html .. "<td>" .. tostring(i) .. "</td>"
    end
  end
  table_html = table_html .. "</tr></table>"
  return table_html
end
