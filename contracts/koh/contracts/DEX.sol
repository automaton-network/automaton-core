pragma solidity ^0.6.2;

library DEX {
  enum OrderType {None, Buy, Sell, Auction}

  struct Order {
    uint256 AUTO;
    uint256 ETH;
    address payable owner;
    OrderType orderType;
  }

  struct Data {
    mapping(uint256 => Order) orders;
    mapping(address => uint256) balanceETH;
    uint256 ids;
  }

  function removeOrder(Data storage self, uint256 _id) public {
    Order memory o;
    o.AUTO = 0;
    o.ETH = 0;
    o.owner = address(0);
    o.orderType = OrderType.None;

    self.orders[_id] = o;
  }

  function addOrder(Data storage self,
                    uint256 _AUTO,
                    uint256 _value,
                    address payable _sender,
                    OrderType _orderType) public returns (uint256) {
    uint256 id = ++self.ids;
    self.orders[id] = Order(_AUTO, _value, _sender, _orderType);
    return id;
  }

  function sellNow(Data storage self, uint256 _id, uint256 _AUTO, uint256 _ETH) public {
    Order memory o = self.orders[_id];
    require(o.owner != address(0), "Invalid Order ID");
    require(o.AUTO == _AUTO, "Order AUTO does not match requested size");
    require(o.ETH == _ETH, "Order ETH does not match requested size");
    require(o.orderType == OrderType.Buy, "Invalid order type");
    uint256 balance = self.balanceETH[msg.sender];
    require(balance + _ETH > balance);
    self.balanceETH[msg.sender] += _ETH;
    removeOrder(self, _id);
  }

  function buyNow(Data storage self, uint256 _id, uint256 _AUTO, uint256 _value) public {
    Order memory o = self.orders[_id];
    require(o.owner != address(0), "Invalid Order ID");
    require(o.AUTO == _AUTO, "Order AUTO does not match requested size");
    require(o.ETH == _value, "Order ETH does not match requested size");
    require(o.orderType == OrderType.Sell, "Invalid order type");
    uint256 balance = self.balanceETH[o.owner];
    require(balance + _value > balance);
    self.balanceETH[o.owner] += _value;
    removeOrder(self, _id);
  }
}
