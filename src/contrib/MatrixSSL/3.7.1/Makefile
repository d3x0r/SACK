# Recursively build the directories below
# e6a3d9f (HEAD, tag: MATRIXSSL-3-7-1-OPEN, origin/master, origin/HEAD, master)
all:
	$(MAKE) --directory=core
	$(MAKE) --directory=crypto
	$(MAKE) --directory=matrixssl
	
gold:
	$(MAKE) gold --directory=core
	$(MAKE) gold --directory=crypto
	$(MAKE) gold --directory=matrixssl
	
clean:
	$(MAKE) clean --directory=core
	$(MAKE) clean --directory=crypto
	$(MAKE) clean --directory=matrixssl
