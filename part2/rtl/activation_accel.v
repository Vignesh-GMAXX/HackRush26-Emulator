module activation_accel (
    input  wire         clk,
    input  wire         rst_n,
    input  wire         valid_i,
    input  wire [1:0]   op_sel_i,
    input  wire signed [31:0] x_i,
    output reg          valid_o,
    output reg signed [31:0] y_o
);

    reg signed [31:0] y_next;

    always @(*) begin
        case (op_sel_i)
            2'b00: begin
                y_next = (x_i > 0) ? x_i : 32'sd0;
            end
            2'b01: begin
                y_next = (x_i >= 0) ? x_i : (x_i >>> 3);
            end
            2'b10: begin
                if (x_i <= -32'sd16384) begin
                    y_next = 32'sd0;
                end else if (x_i >= 32'sd49150) begin
                    y_next = 32'sd32767;
                end else begin
                    y_next = (x_i + 32'sd16384) >>> 1;
                end
            end
            default: begin
                y_next = x_i;
            end
        endcase
    end

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            valid_o <= 1'b0;
            y_o <= 32'sd0;
        end else begin
            valid_o <= valid_i;
            if (valid_i) begin
                y_o <= y_next;
            end
        end
    end

endmodule
