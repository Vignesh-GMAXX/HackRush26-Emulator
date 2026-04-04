`timescale 1ns/1ps

module activation_accel_tb;
    reg clk;
    reg rst_n;
    reg valid_i;
    reg [1:0] op_sel_i;
    reg signed [31:0] x_i;
    wire valid_o;
    wire signed [31:0] y_o;

    integer errors;

    activation_accel dut (
        .clk(clk),
        .rst_n(rst_n),
        .valid_i(valid_i),
        .op_sel_i(op_sel_i),
        .x_i(x_i),
        .valid_o(valid_o),
        .y_o(y_o)
    );

    always #5 clk = ~clk;

    task drive_and_check;
        input [1:0] op;
        input signed [31:0] x;
        input signed [31:0] expected;
        begin
            @(negedge clk);
            valid_i = 1'b1;
            op_sel_i = op;
            x_i = x;

            @(negedge clk);
            valid_i = 1'b0;

            @(posedge clk);
            if (!valid_o) begin
                $display("[FAIL] valid_o low for op=%0d x=%0d", op, x);
                errors = errors + 1;
            end else if (y_o !== expected) begin
                $display("[FAIL] op=%0d x=%0d y=%0d expected=%0d", op, x, y_o, expected);
                errors = errors + 1;
            end else begin
                $display("[PASS] op=%0d x=%0d y=%0d", op, x, y_o);
            end
        end
    endtask

    initial begin
        clk = 0;
        rst_n = 0;
        valid_i = 0;
        op_sel_i = 2'b00;
        x_i = 0;
        errors = 0;

        repeat (3) @(posedge clk);
        rst_n = 1;

        drive_and_check(2'b00, -32'sd7, 32'sd0);
        drive_and_check(2'b00,  32'sd9, 32'sd9);
        drive_and_check(2'b01, -32'sd16, -32'sd2);
        drive_and_check(2'b01,  32'sd16,  32'sd16);
        drive_and_check(2'b10, -32'sd20000, 32'sd0);
        drive_and_check(2'b10,  32'sd0, 32'sd8192);

        if (errors == 0) begin
            $display("ALL TESTS PASSED");
        end else begin
            $display("TESTS FAILED, errors=%0d", errors);
        end
        $finish;
    end
endmodule
