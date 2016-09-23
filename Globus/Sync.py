from GridFTPConnection import GridFTPConnection
from Operations import Operations
from FolderWatchdog import FolderWatchdog
from config import Config
from watchdog.observers import Observer
from watchdog.events import *
import argparse, logging, posixpath, sys, time


logFormat='%(levelname)s: %(message)s'
logging.basicConfig(format=logFormat)
logger = logging.getLogger('Sync')
logger.setLevel(logging.INFO)


def _main():

  parser = argparse.ArgumentParser(description='Reflect local filesystem changes on a remote system in real time, automatically.')
  parser.add_argument('-c', '--config-file', default='sync.cfg', help='location of a sync configuration file')
  args = parser.parse_args()

  try:
    config_file = file(args.config_file)
  except Exception as e:
    logger.error('Couldn\'t read sync configuration file!\n\
    Either place a sync.cfg file in the same folder as sync.py, or specify an alternate location.\n\
    Run \'%s -h\' for usage information.\nCause: %s' % (os.path.basename(__file__), e))
    sys.exit(1)

  try:
    cfg = Config(config_file)
  except Exception as e:
    logger.error('Sync configuration file is invalid!\nCause: %s' % e)
    sys.exit(1)

  # Read configuration
  local_root_path = os.path.abspath(os.path.expanduser(cfg.local_root_path))
  if not os.path.isdir(local_root_path):
    logger.error('Invalid local_root_path configured: %s is not a valid path on the local machine' % cfg.local_root_path)
    sys.exit(1)
  else:
    logger.debug('Using local root path: ' + local_root_path)

  gridftp_connection = GridFTPConnection(cfg.endpoint,cfg.source)

  logger.debug('Initializating path mappings...')

  no_valid_mappings = True

  observer = Observer()

  for mapping in cfg.path_mappings:

    # Create an absolute local path from the local root path and this mapping's local relative path
    local_base = os.path.join(local_root_path, mapping.local)
    if not os.path.isdir(local_base):
      logger.warn('Invalid path mapping configured: %s is not a valid path on the local machine' % local_base)
      continue

    no_valid_mappings = False

    remote_base = posixpath.join(cfg.remote_root_path, mapping.remote)

    logger.info('Path mapping initializing:\n'
                'Changes at local path\n\t%s\n'
                'will be reflected at remote path\n\t%s'
                % (local_base, remote_base))

    # Create necessary objects for this particular mapping and schedule this mapping on the Watchdog observer as appropriate
    operations = Operations(gridftp_connection = gridftp_connection, local_base = local_base, remote_base = remote_base)
    #Create an initial sync
    print "Syncing folders"

    operations.transfer_file(local_base)

    event_handler = FolderWatchdog(ignore_patterns = cfg.ignore_patterns, operations = operations)
    observer.schedule(event_handler, path=local_base, recursive=True)

  if no_valid_mappings:
    logger.error('No valid path mappings were configured, so there\'s nothing to do. Please check your sync configuration file.')
    sys.exit('Terminating.')
  observer.start()

  try:
    while True:
      time.sleep(1)
  except KeyboardInterrupt:
    observer.stop()
  observer.join()

if __name__ == "__main__":

  _main()