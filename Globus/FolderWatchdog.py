from watchdog.events import *
import logging


logFormat='%(levelname)s: %(message)s'
logging.basicConfig(format=logFormat)
logger = logging.getLogger('Sync')
logger.setLevel(logging.INFO)


class FolderWatchdog(PatternMatchingEventHandler):
  """
  Watchdog event handler.
  Triggers appropriate actions on a remote server via Operations oblject when
  specific Watchdog events are fired due to local filesystem changes.
  """

  def __init__(self, operations, **kw):
    super(FolderWatchdog, self).__init__(**kw)
    self._operations = operations

  def on_created(self, event):
    '''Method to be called when new file create event is triggered'''
    if isinstance(event, DirCreatedEvent):
      logger.info('Ignoring DirCreatedEvent for %s' % event.src_path)
    else:
      logger.info('Creating %s' % event.src_path)
      self._operations.transfer_file(event.src_path)

  def on_deleted(self, event):
    '''Method to be called when a file is deleted'''
    #TODO:Add support for file deletion uisng the Globus API.
    pass

  def on_modified(self, event):
    '''Method called when file modified event is triggered'''
    if isinstance(event, DirModifiedEvent):
      logger.info('Ignoring DirModifiedEvent for %s' % event.src_path)
    else:
      logger.info('Modyfying %s' % event.src_path)
      self._operations.transfer_file(event.src_path)

  def on_moved(self, event):
    '''Method to be called when on modified event is triggered'''
    logger.info('Moving %s' % event.src_path)
    logger.info('Moving %s' % event.dest_path)