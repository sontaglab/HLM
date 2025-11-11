// Benchmark "rca2" written by ABC on Tue May 12 02:15:38 2020

module rca2 ( 
    a0, b0, a1, b1,
    s0, s1, s2  );
  input  a0, b0, a1, b1;
  output s0, s1, s2;
  wire new_n8_, new_n9_, new_n11_, new_n12_, new_n13_, new_n14_, new_n15_,
    new_n16_, new_n18_, new_n19_;
  assign new_n8_ = ~a0 & b0;
  assign new_n9_ = a0 & ~b0;
  assign s0 = new_n8_ | new_n9_;
  assign new_n11_ = a0 & b0;
  assign new_n12_ = ~a1 & b1;
  assign new_n13_ = a1 & ~b1;
  assign new_n14_ = ~new_n12_ & ~new_n13_;
  assign new_n15_ = new_n11_ & new_n14_;
  assign new_n16_ = ~new_n11_ & ~new_n14_;
  assign s1 = new_n15_ | new_n16_;
  assign new_n18_ = a1 & b1;
  assign new_n19_ = new_n11_ & ~new_n14_;
  assign s2 = new_n18_ | new_n19_;
endmodule


