class Tee:
    """Helper class for writing to different sockets/files at the same time
    """
    def __init__(self, *sockets):
        self.sockets = sockets

    def write(self, obj):
        for s in self.sockets:
            s.write(obj)
            s.flush()

    def flush(self):
        for s in self.sockets:
            s.flush()