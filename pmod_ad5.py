#   Copyright (c) 2016, Xilinx, Inc.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without 
#   modification, are permitted provided that the following conditions are met:
#
#   1.  Redistributions of source code must retain the above copyright notice, 
#       this list of conditions and the following disclaimer.
#
#   2.  Redistributions in binary form must reproduce the above copyright 
#       notice, this list of conditions and the following disclaimer in the 
#       documentation and/or other materials provided with the distribution.
#
#   3.  Neither the name of the copyright holder nor the names of its 
#       contributors may be used to endorse or promote products derived from 
#       this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
#   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


from math import ceil
from . import Pmod
from . import MAILBOX_OFFSET

__author__ = "Reynaldo Gonzalez"
__copyright__ = ""
__email__ = "reynaldogonzalez@protonmail.com"


PMOD_AD5_PROGRAM = "pmod_ad5.bin"
PMOD_AD5_LOG_START = MAILBOX_OFFSET+16
PMOD_AD5_LOG_END = PMOD_AD5_LOG_START+(1000*4)

RESET = 0x1
READ_SINGLE_VALUE = 0x3
STATUS = 0x7
READ_CONFIGURATION = 0x9
READ_MODE = 0x11


class Pmod_AD5(object):
    """This class controls AD5 pmod

    Attributes
    ----------
    microblaze : Pmod
        Microblaze processor instance used by this module.
    log_interval_ms : int
        Time in milliseconds between sampled reads.
        
    """
    def __init__(self, mb_info):
        """Return a new instance of an AD5 object. 
        
        Parameters
        ----------
        mb_info : dict
            A dictionary storing Microblaze information, such as the
            IP name and the reset name.

        """
        self.microblaze = Pmod(mb_info, PMOD_AD5_PROGRAM)

    def reset(self):
        """Reset the ADC.
        
        Returns
        -------
        None
        
        """
        self.microblaze.write_blocking_command(RESET)

    def status(self):
        """Return the status of the ADC
        
        Returns
        -------
        Status 8-bit
        """
        self.microblaze.write_blocking_command(STATUS)
        data = self.microblaze.read_mailbox(0)
        return data
        

    def read(self):
        """Read voltage measured by AD5 pmod
        
        Returns
        -------
        int
            The voltage value
        
        """
        self.microblaze.write_blocking_command(READ_SINGLE_VALUE)
        data = self.microblaze.read_mailbox(0)
        return data

    def read_config(self):
        """Return contents of configurations
        
        Returns
        -------
        int
            The configuration register
        
        """
        self.microblaze.write_non_blocking_command(READ_CONFIGURATION)
        data = self.microblaze.read_mailbox(0)
        return data

    def READ_MODE(self):
        """Return contents of mode reg.
        
        Returns
        -------
        int
            The mode register
        
        """
        self.microblaze.write_non_blocking_command(READ_MODE)
        data = self.microblaze.read_mailbox(0)
        return data