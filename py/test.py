#!/bin/env python3


from dali.gear.general import DTR0, SearchaddrH, Terminate, Initialise, ProgramShortAddress, VerifyShortAddress



print(f"DTR0 set is {DTR0(12).frame.as_integer:x}")
print(f"SetSearchAddrH set is {SearchaddrH(127).frame.as_integer:x}")
print(f"Terminate set is {Terminate().frame.as_integer:x}")
print(f"Initialise(true) set is {Initialise(broadcast=True).frame.as_integer:x}")
print(f"Initialise(false) set is {Initialise(broadcast=False).frame.as_integer:x}")
print(f"ProgramShortAddress set is {ProgramShortAddress(0x11).frame.as_integer:x}")
print(f"VerifyShortAddress set is {VerifyShortAddress(0x22).frame.as_integer:x}")



