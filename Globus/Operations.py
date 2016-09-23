from globus_sdk import AuthClient, TransferClient, TransferData
from globus_sdk import ConfidentialAppAuthClient
from globus_sdk import RefreshTokenAuthorizer
from watchdog.events import *


logFormat='%(levelname)s: %(message)s'
logging.basicConfig(format=logFormat)
logger = logging.getLogger('Sync')
logger.setLevel(logging.INFO)


class Operations:
    """
    Performs filesystem manipulations on a remote machine using the globus API,
    using data from the local machine's filesystem as necessary.
    """

    def __init__(self, gridftp_connection, local_base, remote_base):
        self._connection = gridftp_connection.transfer_client
        self._endpointid = gridftp_connection.endpoint_id
        self._sourceid = gridftp_connection.source_id
        self._local_base = local_base
        self._remote_base = remote_base
        self._local_base_length = len(local_base)

    def _get_remote_path(self, local_path):
        '''This creates the remote path from the local path'''
        remote_relative = local_path[self._local_base_length+1:]
        return self._remote_base + remote_relative

    def transfer_file(self, src_path):
        ''' Transfers file from the local machine to the remote machine'''
        dest_path = self._get_remote_path(src_path)
        isDir = os.path.isdir(src_path)
        logger.info('Copying\n\t%s\nto\n\t%s' % (src_path.replace('\\', '/'), dest_path.replace('\\', '/')))
        try:
            dest_path = dest_path.replace('\\', '/')
            head_d, tail_d = os.path.split(dest_path)
            src_path = src_path.replace('\\', '/')
            head_s,tail_s = os.path.split(dest_path)

            if os.access(src_path, os.R_OK):
                temp,src_path = src_path.split(":/")
                tdata = TransferData(self._connection,
                                     self._sourceid,
                                     self._endpointid,
                                     label= re.sub('[^A-Za-z0-9]+', '', tail_s),
                                     sync_level="exists")
                #tdata.add_item(head_s, head_d,recursive=True)
                print("Creting dir src:%s dir:%s" % (head_s, head_d))
                tdata.add_item(src_path, dest_path, recursive=isDir)
                print("Creting dir src:%s dir:%s" % (src_path, dest_path))
                transfer_result = self._connection.submit_transfer(tdata)
                print("task_id =", transfer_result["task_id"])
                print("code =", transfer_result["code"])
                print("message =", transfer_result["message"])

        except Exception as e:
            logger.error('Caught exception while copying:')
            logger.exception(e)

    def delete_resource(self, src_path):
        '''Use this method to submit a delete task using the globus API'''
        pass
