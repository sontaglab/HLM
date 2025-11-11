module majority3 (
    input a,
    input b,
    input c,
    output y
);
  assign y = (a & b) | (a & c) | (b & c);
endmodule
