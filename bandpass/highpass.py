import math
import vertica_sdk

"""
An implementation of the High-pass filter as described by
https://en.wikipedia.org/wiki/High-pass_filter 

For installation, setup, and use, see the README.

"""

class highpass(vertica_sdk.ScalarFunction):
    """Apply a high-pass filter to a column sorted by timestamp
       For each row, pass in:
        - float freq (cutoff frequency, a constant)
        - float delta_t
        - float x, the time-series variable being filtered

        Wikipedia describes a high-pass filter here: https://en.wikipedia.org/wiki/High-pass_filter 

        So, the Wikipedia explanation of the algorithm uses a constant
        alpha, derived from the RC-circuit origin of the high-pass
        filter analysis.  But people really want to think about
        frequency, especially if they're not working on an RC circuit.

        Fortunately, the Wikipedia page works this out for us, 

        - freq = (1 - alpha)/(2 * pi * alpha * delta_t)
        - alpha = 1/(1 + 2*pi*delta_t*freq)

        In the discrete high-pass filter calculation, 
        
        for t in the time-series:
            y[t] = alpha * (y[t-1] - x[t-1] + x[t])

    """

    def __init__(self):
        self.y_last = 0
        self.x_last = 0
        self.first_row = True

    def setup(self, server_interface, col_types):
        self.y_last = 0
        self.x_last = 0
        self.first_row = True
        server_interface.log("high-pass filter: setup called")

    def processBlock(self, server_interface, arg_reader, res_writer):
        # Writes a string to the UDx log file.
        server_interface.log("Python UDx - high-pass filter!")
        while(True):
            # Example of error checking best practices.
            y = 0
            if arg_reader.isNull(0) or arg_reader.isNull(1) or arg_reader.isNull(2):
                server_interface.log("high-pass filter: Null input")
            else:
                freq = arg_reader.getFloat(0)
                delta_t = arg_reader.getFloat(1)
                val = arg_reader.getFloat(2)
                alpha = 1/(1 + (2 * math.pi * delta_t * freq))
                if self.first_row:
                    server_interface.log("high-pass filter: First row")
                    y = val
                    self.x_last = val
                    self.first_row = False
                else:
                    y = alpha * (self.y_last - self.x_last + val)
                    self.x_last = val
                    self.y_last = y
                                 
            res_writer.setFloat(y)
            res_writer.next() # Read the next row.
            if not arg_reader.next():
                # Stop processing when there are no more input rows.
                server_interface.log("high-pass filter: end of block")
                break
    def destroy(self, server_interface, col_types):
        pass

class highpass_factory(vertica_sdk.ScalarFunctionFactory):
    
    def createScalarFunction(self, srv):
        return highpass()

    def getPrototype(self, srv_interface, arg_types, return_type):
        arg_types.addFloat()
        arg_types.addFloat()
        arg_types.addFloat()
        return_type.addFloat()

    def getReturnType(self, srv_interface, arg_types, return_type):
        return_type.addFloat()
