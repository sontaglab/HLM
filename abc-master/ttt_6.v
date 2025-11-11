module rca2 (x_3 ,x_4, x_5, s);
//-------------Input Ports Declarations-----------------------------
input x_3 ,x_4, x_5;
//-------------Output Ports Declarations-----------------------------
output s;
//-------------Logic-----------------------------------------------
assign s = (x_3 & x_4 & x_5);
endmodule
