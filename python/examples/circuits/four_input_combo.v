module four_input_combo (
    input a,
    input b,
    input c,
    input d,
    output y
);
  assign y = ((~a & b) | (c & d)) ^ (b & ~c);
endmodule
