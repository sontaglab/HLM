module rca2 (x_3, x_4, x_6, x_7, x_9, s);
//-------------Input Ports Declarations-----------------------------
input x_3, x_4, x_6, x_7, x_9;
//-------------Output Ports Declarations-----------------------------
output s;
//-------------Logic-----------------------------------------------
assign s = ((x_4 | x_7) & x_3 & (x_6 | x_9));
endmodule
