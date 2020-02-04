pragma solidity ^0.6.2;
pragma experimental ABIEncoderV2;

contract test_contract {
  constructor() public {
    alabala = 34;
  }

  int256 alabala;

  function f1(uint32 a) public pure returns (uint256) {
    return a + 256;
  }

  function f2(uint8 a) public pure returns (uint256[] memory result) {
    result = new uint[](a);
    for(uint256 i = 0; i < a; i++) {
      result[i] = i*i;
    }
  }

  function f3(int40 a, int32 b) public pure returns (int32 c, int40 d) {
    c = b;
    d = a;
  }

  function f4(string memory a) public pure returns (string memory result) {
    bytes memory a_bytes = bytes(a);
    bytes memory res = new bytes(a_bytes.length);
    for(uint16 i = 0; i < a_bytes.length; i++) {
      res[i] = a_bytes[a_bytes.length - i - 1];
    }
    result = string(res);
  }

  function f5() public pure returns (bytes5 result) {
    result = 0xABCDEABCDE;
  }

  function f6() public view returns (address) {
    return address(this);
  }

  function f7(bool a) public pure returns (bool) {
    return !a;
  }

  function f8(int a) public pure returns (int) {
    return -a;
  }

  function f9(int8 a) public pure returns (int16) {
    return 2 * a;
  }

  function f10(uint16[2][3] memory a) public pure returns (uint b) {
    b = 1;
    for (uint16 i = 0; i < 3; ++i) {
      for (uint16 j = 0; j < 2; ++j) {
        b *= a[i][j];
      }
    }
  }

  function f11(uint16[][] memory a) public pure returns (uint b) {
    b = 1;
    for (uint16 i = 0; i < a.length; ++i) {
      for (uint16 j = 0; j < a[i].length; ++j) {
        b *= a[i][j];
      }
    }
  }

  function f12(uint[5] memory a) public pure returns (uint b) {
    b = 0;
    for (uint16 i = 0; i < 5; ++i) {
      b += a[i];
    }
  }

  function f13() public pure returns (uint[][] memory a) {
    a = new uint[][](2);
    for (uint16 i = 0; i < 2; ++i) {
      a[i] = new uint[](3);
      for (uint16 j = 0; j < 3; ++j) {
        a[i][j] = i + j;
      }
    }
  }

  function f14() public pure returns (uint[3][2] memory a) {
    for (uint16 i = 0; i < 2; ++i) {
      for (uint16 j = 0; j < 3; ++j) {
        a[i][j] = i + j;
      }
    }
  }

  function f15() public pure returns (uint[3][] memory a) {
    a = new uint[3][](2);
    for (uint16 i = 0; i < 2; ++i) {
      for (uint16 j = 0; j < 3; ++j) {
        a[i][j] = i + j;
      }
    }
  }

  function f16() public pure returns (uint[][2] memory a) {
    for (uint16 i = 0; i < 2; ++i) {
      a[i] = new uint[](3);
      for (uint16 j = 0; j < 3; ++j) {
        a[i][j] = i + j;
      }
    }
  }

  function f17(uint16[3][] memory a) public pure returns (uint16[][3] memory b) {
    for (uint16 i = 0; i < 3; ++i) {
      b[i] = new uint16[](2);
      for (uint16 j = 0; j < 2; ++j) {
        b[i][j] = a[j][i];
      }
    }
  }

  function f18(uint16[][3] memory a) public pure returns (uint16[3][] memory b) {
    b = new uint16[3][](2);
    for (uint16 i = 0; i < 2; ++i) {
      for (uint16 j = 0; j < 3; ++j) {
        b[i][j] = a[j][i];
      }
    }
  }

  function f19(string[3][] memory a) public pure returns (string[][3] memory b) {
    for (uint16 i = 0; i < 3; ++i) {
      b[i] = new string[](2);
      for (uint16 j = 0; j < 2; ++j) {
        b[i][j] = a[j][i];
      }
    }
  }

  function f20(string[][3] memory a) public pure returns (string[3][] memory b) {
    b = new string[3][](2);
    for (uint16 i = 0; i < 2; ++i) {
      for (uint16 j = 0; j < 3; ++j) {
        b[i][j] = a[j][i];
      }
    }
  }

  function f21(uint256[4][2] memory a, string[][2][2] memory b) public pure returns
      (uint[4][1][2] memory c, bool d, string[][4] memory e) {
    for (uint16 i = 0; i < 4; ++i) {
      c[0][0][i] = a[1][i];
      c[1][0][i] = a[0][i];
    }

    d = true;

    e[0] = b[0][0];
    e[1] = b[0][1];
    e[2] = b[1][0];
    e[3] = b[1][1];
  }

  function f22(string[][2][] memory a) public pure returns (string[][2][] memory b) {
    // a -> [1][2][3], b -> [3][2][1]
    b = new string[][2][](1);
    b[0][0] = new string[](3);
    b[0][1] = new string[](3);
    for (uint16 i = 0; i < 3; ++i) {
      b[0][0][i] = a[i][0][0];
      b[0][1][i] = a[i][1][0];
    }
  }

}
