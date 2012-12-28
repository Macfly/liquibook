Performance Test Results, Inserts Per Second

<table>
  <tr>
    <th>5 Level Depth</th>
    <th>BBO Only</th>
    <th>Order Book Only</th>
    <th>Note</th>
  </tr>
  <tr>
    <td>1,267,135</td>
    <td>1,270,188</td>
    <td>1,469,246</td>
    <td>Combine 2 fill callbacks into one.</td>
  </tr>
  <tr>
    <td>1,233,894</td>
    <td>1,237,154</td>
    <td>1,434,354</td>
    <td>Store excess depth levels in depth to speed repopulation.</td>
  </tr>
  <tr>
    <td>58,936</td>
    <td>153,839</td>
    <td>1,500,874</td>
    <td>Removed spuroious insert on accept of completely filled order.</td>
  </tr>
  <tr>
    <td>38,878</td>
    <td>124,756</td>
    <td>1,495,744</td>
    <td>Initial run with all 3 tests.</td>
  </tr>
</table>

