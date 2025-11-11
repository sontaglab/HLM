module rca2 (x_2, x_3, x_4, x_5, x_6, x_8, x_9, s);
//-------------Input Ports Declarations-----------------------------
input x_2, x_3, x_4, x_5, x_6, x_8, x_9;
//-------------Output Ports Declarations-----------------------------
output s;
//-------------Logic-----------------------------------------------
assign s = ((x_2 & x_5) | (x_3 & (x_6 | x_8 | x_9))) & (~x_4);
endmodule
