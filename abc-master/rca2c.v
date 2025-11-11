// Benchmark "rca2" written by ABC on Tue May 12 15:25:55 2020

module rca2 ( 
    a, b, c, d,
    s  );
  input  a, b, c, d;
  output s;
  wire new_n6_, new_n7_;
  assign new_n6_ = ~a & ~d;
  assign new_n7_ = c & new_n6_;
  assign s = b & ~new_n7_;
endmodule


