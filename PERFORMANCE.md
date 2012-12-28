Performance Test Results, Inserts Per Second

<table>
  <tr>
    <th>5 Level Depth</th>
    <th>BBO Only</th>
    <th>Order Book Only</th>
    <th>Note</th>
  </tr>
  <tr>
    <td>1,270,114</td>
    <td>1,293,720</td>
    <td>1,509,442</td>
    <td>After storing excess depth levels in depth</td>
  </tr>
  <tr>
    <td>58,936</td>
    <td>153,839</td>
    <td>1,500,874</td>
    <td>After removing spuroious insert on accept of completely filled order</td>
  </tr>
  <tr>
    <td>38,878</td>
    <td>124,756</td>
    <td>1,495,744</td>
    <td>Initial run with all 3 tests</td>
  </tr>
</table>

