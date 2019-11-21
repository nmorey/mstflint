#######################################################
# 
# MlxResDump.py
# Python implementation of the Class MlxResDump
# Generated by Enterprise Architect
# Created on:      14-Aug-2019 10:11:58 AM
# Original author: talve
# 
#######################################################
import sys
import os

if sys.version_info[0] < 3:
    print("Error: This tool supports python 3.x only. Exiting...")
    exit(1)

import argparse
from commands.CommandFactory import CommandFactory
from utils import constants as cs
sys.path.append(os.path.join("common"))
import tools_version


class MlxResDump:
    """This class is responsible for the resource dump UI by handling the user inputs and
       and running the right command.
    """
    @staticmethod
    def _decimal_hex_check(inp):
        """This method check if the string input is hex or decimal.
           if the input is hex the method convert it to decimal number
           otherwise convert it to decimal number.
        """
        try:
            if inp.startswith("0x"):
                inp = int(inp, 16)
            else:
                inp = int(inp)
            return inp
        except Exception as _:
            msg = "wrong value: {0} is not decimal or hex".format(inp)
            raise argparse.ArgumentTypeError(msg)

    @staticmethod
    def _decimal_hex_to_str_hex(inp):
        """This method check if the string input is hex or decimal.
           and convert it to hex number.
           in case that the input is not a number, the method will
           return the input as is (str).
        """
        try:
            if inp.startswith("0x"):
                inp = hex(int(inp, 16))
            else:
                inp = hex(int(inp))
            return str(inp)
        except Exception as _:
            return inp

    @staticmethod
    def _num_of_objs_check(inp):
        """This method check if the num of objects parameter is valid
        """
        if inp in (['all', 'active']):
            if inp == 'all':
                inp = cs.NUM_OF_OBJ_ALL
            else:
                inp = cs.NUM_OF_OBJ_ACTIVE
            return inp
        else:
            try:
                if inp.startswith("0x"):
                    inp = int(inp, 16)
                else:
                    inp = int(inp)
                return inp
            except Exception as _:
                msg = "numOfObj accepts the following values: ['all', 'active', number]"
                raise argparse.ArgumentTypeError(msg)

    @staticmethod
    def _depth_check(inp):
        """This method check if the num depth parameter is valid
        """
        if inp == "inf":
            return inp
        else:
            try:
                inp = int(inp)
                return inp
            except Exception as _:
                msg = "depth accepts the following values: ['inf', number]"
                raise argparse.ArgumentTypeError(msg)

    def run(self):
        # main parser
        tool_name = os.path.basename(__file__.split('.')[0])
        parser = argparse.ArgumentParser(epilog="Use '{0} <command> -h' to read about a specific command.".format(tool_name))
        parser.add_argument('-v', '--version', action='version', help='Shows tool version',
                            version=tools_version.GetVersionString(cs.TOOL_NAME, None))

        # commands sub parser
        commands = parser.add_subparsers(title='commands')

        # dump sub parser
        dump_parser = commands.add_parser(cs.RESOURCE_DUMP_COMMAND_TYPE_DUMP)
        dump_parser.set_defaults(parser=cs.RESOURCE_DUMP_COMMAND_TYPE_DUMP)

        # required arguments by dump sub parser
        dump_required_args = dump_parser.add_argument_group('required arguments')
        dump_required_args.add_argument(cs.UI_DASHES + cs.UI_ARG_DEVICE, cs.UI_DASHES_SHORT + cs.UI_ARG_DEVICE_SHORT,
                                        help='The device name', required=True)
        dump_required_args.add_argument(cs.UI_DASHES + cs.UI_ARG_SEGMENT, help='The segment to dump', required=True,
                                        type=self._decimal_hex_to_str_hex)

        dump_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_VHCAID.replace("_", "-"),
                                 help='The virtual HCA (host channel adapter, NIC) ID')
        dump_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_INDEX1,
                                 help='The first context index to dump (if supported for this segment)',
                                 type=self._decimal_hex_check)
        dump_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_INDEX2,
                                 help='The second context index to dump (if supported for this segment)',
                                 type=self._decimal_hex_check)
        dump_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_NUMOFOBJ1.replace("_", "-"),
                                 help='The number of objects to be dumped (if supported for this segment). accepts: ["all", "active", number, depends on the capabilities]',
                                 type=self._num_of_objs_check)
        dump_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_NUMOFOBJ2.replace("_", "-"),
                                 help='The number of objects to be dumped (if supported for this segment). accepts: ["all", "active", number, depends on the capabilities]',
                                 type=self._num_of_objs_check)
        dump_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_DEPTH,
                                 help='The depth of walking through reference segments. 0 stands for flat, '
                                      '1 allows crawling of a single layer down the struct, etc. "inf" for all',
                                 type=self._depth_check)
        dump_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_BIN,
                                 help='The output to a binary file that replaces the default print in hexadecimal,'
                                      ' a readable format')

        # query sub parser
        query_parser = commands.add_parser(cs.RESOURCE_DUMP_COMMAND_TYPE_QUERY)
        query_parser.set_defaults(parser=cs.RESOURCE_DUMP_COMMAND_TYPE_QUERY)
        query_parser.add_argument(cs.UI_DASHES + cs.UI_ARG_VHCAID.replace("_", "-"),
                                  help='The virtual HCA (host channel adapter, NIC) ID')

        # required arguments by query sub parser
        query_required_args = query_parser.add_argument_group('required arguments')
        query_required_args.add_argument(cs.UI_DASHES + cs.UI_ARG_DEVICE, cs.UI_DASHES_SHORT + cs.UI_ARG_DEVICE_SHORT,
                                         help='The device name', required=True)

        # args = parser.parse_args(
        #     'dump -d test --source test2 --vHCAid 15 --index 1 --index2 2 --numOfObj1 10 --numOfObj2 20
        #           --depth inf --bin tt.txt'.split())
        arguments = parser.parse_args()
        return arguments


def create_command(arguments):
    """This method creates the right command.
    """
    try:
        command_type = arguments.parser
        created_command = CommandFactory.create(command_type, **vars(arguments))
        return created_command
    except Exception as exp:
        raise Exception("failed to create command of type: {0} with exception: {1}".format(command_type, exp))


if __name__ == '__main__':
    try:
        args = MlxResDump().run()
        command = create_command(args)
        command.execute()
    except Exception as e:
        print("Error: {0}. Exiting...".format(e))
        sys.exit(1)
