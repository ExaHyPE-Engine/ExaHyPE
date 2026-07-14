# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import sys
import logging

_COLOR_CODES = {
    logging.DEBUG: "\033[90m",     # Grey
    logging.INFO: "\033[32m",      # Green
    logging.WARNING: "\033[33m",   # Yellow
    logging.ERROR: "\033[31m",     # Red
    logging.CRITICAL: "\033[1;35m" # Bright Magenta
}
_RESET_CODE = "\033[0m"


class _SimpleColorFormatter(logging.Formatter):
    def format(self, record):
        color = _COLOR_CODES.get(record.levelno, "")
        message = super().format(record)
        return f"{color}{message}{_RESET_CODE}"


def setup_logging(level=logging.INFO):
    formatter = _SimpleColorFormatter("[%(levelname)s]: %(message)s")
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(formatter)

    root_logger = logging.getLogger()
    root_logger.setLevel(level)
    root_logger.handlers = [handler]

log = logging.getLogger()
