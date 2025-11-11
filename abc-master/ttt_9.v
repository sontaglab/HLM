module rca2 (x_2, x_3 ,x_4, x_7, x_8, s);
//-------------Input Ports Declarations-----------------------------
input x_2, x_3 ,x_4, x_7, x_8;
//-------------Output Ports Declarations-----------------------------
output s;
//-------------Logic-----------------------------------------------
assign s = ((x_2 & x_7) | x_3 & ((x_4 & x_7) | (x_8 & (x_4 | x_7))));
endmodule
