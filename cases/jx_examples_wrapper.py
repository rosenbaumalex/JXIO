#!/usr/bin/env python

# Built-in modules
import sys

from reg2_wrapper.test_wrapper.client_server_wrapper import ClientServerWrapper

class JxWrapper(ClientServerWrapper):

    def get_server_prog_path(self):
        return "../examples/runHelloWorld.sh server"

    def get_client_prog_path(self):
        return "../examples/runHelloWorld.sh client"

    def configure_parser(self):
        super(JxWrapper, self).configure_parser()
 
        # Arguments
        self.add_client_cmd_argument('--ip_address_client', help='The server IP.', type=str, value_only=True, priority=1)
        self.add_client_cmd_argument('--port_client', help='The server port', type=str, value_only=True, priority=2)

        self.add_server_cmd_argument('--ip_address_server', help='The server IP.', type=str, value_only=True, priority=1)
        self.add_server_cmd_argument('--port_server', help='The server port', type=str, value_only=True, priority=2)

#     def get_server_manage_ip(self):
#         return self.ServerPlayer.Ip
# 
#     def get_client_manage_ip(self):
#         return self.ClientPlayers[0].Ip
# 
#     def get_client_test_ip(self):
#         return self.ClientEPoints[0].ipv4
                              
if __name__ == "__main__":
    wrapper = JxWrapper("JX Wrapper")
    wrapper.execute(sys.argv[1:])

