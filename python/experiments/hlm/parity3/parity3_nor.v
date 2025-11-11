module parity3 (input a, input b, input c, output y);
  wire n0, n1, n2, n3, n4, n5, n6;
  nor NOR_0 (n0, a, c);
  nor NOR_1 (n1, n0, c);
  nor NOR_2 (n2, a, n0);
  nor NOR_3 (n3, n1, n2);
  nor NOR_4 (n4, b, n3);
  nor NOR_5 (n5, n4, n3);
  nor NOR_6 (n6, b, n4);
  nor NOR_7 (y, n5, n6);
endmodule