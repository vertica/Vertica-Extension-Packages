"""
An implementation of the low-pass filter as described by
https://en.wikipedia.org/wiki/Low-pass_filter 

For setup and installation, see the README.

(This is called p_lowpass for "parameterized lowpass", replacing
an earlier lowpass library and function which only used alpha (no
frequency option).)
"""
# (c) Copyright [2022] Micro Focus or one of its affiliates.
# Licensed under the Apache License, Version 2.0 (the "License");
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import math
import vertica_sdk


class p_lowpass(vertica_sdk.ScalarFunction):
    """Apply a low-pass filter to a column sorted by timestamp
       For each row, pass in:
        - float delta-t 
        - float x, the time-series variable being filtered
       USING PARAMETERS [alpha=10, frequency=150])

        Wikipedia describes a low-pass filter here: https://en.wikipedia.org/wiki/Low-pass_filter

        In the discrete low-pass filter calculation, 
    
        for t in the time-series:
            y[t] = y[t-1] + dt/(alpha + dt) * (x[t] - y[t-1])
    """

    def __init__(self):
        self.y_last = 0
        self.first_row = True

    def setup(self, server_interface, col_types):
        self.y_last = 0
        self.first_row = True
        server_interface.log("low-pass filter: setup called")

    def processBlock(self, server_interface, arg_reader, res_writer):
        # Writes a string to the UDx log file.
        server_interface.log("Python UDx - low-pass filter!")
        param_reader = server_interface.getParamReader()
        if param_reader.containsParameter("alpha"):
            alpha_constant = param_reader.getFloat("alpha")
            if alpah_constant < 0 or alpha_constant > 1:
                raise ValueError(f"Alpha {alpha_constant} must be positive and lass than 1")
            # in this case alpha is a constant, independent of dt
            def alpha(dt): return alpha_constant
        elif param_reader.containsParameter("frequency"):
            frequency = param_reader.getFloat("frequency")
            # alpha is a function of both frequency and dt
            # define dt as part of the function's environment
            def alpha(dt):
                return (2 * math.pi * dt * frequency)/((2 * math.pi * dt * frequency) + 1)
        else:
            raise ValueError("Neither alpha nor frequency selected")
        while(True):
            y = 0
            if arg_reader.isNull(0) or arg_reader.isNull(1):
                server_interface.log("low-pass filter: Null input")
            else:
                dt = arg_reader.getFloat(0)
                val = arg_reader.getFloat(1)
                if self.first_row:
                    server_interface.log("low-pass filter: First row")
                    y = val * alpha(dt)
                    self.first_row = False
                else:
                    y = self.y_last + (alpha(dt) * (val - self.y_last))
                self.y_last = y
            res_writer.setFloat(y)
            res_writer.next() # Read the next row.
            if not arg_reader.next():
                # Stop processing when there are no more input rows.
                server_interface.log("low-pass filter: end of block")
                break
    def destroy(self, server_interface, col_types):
        pass

class p_lowpass_factory(vertica_sdk.ScalarFunctionFactory):
    
    def createScalarFunction(self, srv):
        return p_lowpass()

    def getPrototype(self, srv_interface, arg_types, return_type):
        arg_types.addFloat()
        arg_types.addFloat()
        return_type.addFloat()

    def getReturnType(self, srv_interface, arg_types, return_type):
        return_type.addFloat()

    def getParameterType(self, srv_interface, parameter_types):
        parameter_types.addFloat("alpha")
        parameter_types.addFloat("frequency")
