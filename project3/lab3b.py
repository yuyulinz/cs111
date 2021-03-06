#!/usr/bin/python

import csv

errorprint = ""
#superblock
############################################################################
with open("super.csv", "r") as super:
    superfile = csv.reader(super)
    for row in superfile:
        nINODES = int(row[1])
        nBLOCKS = int(row[2])
        blocksize = int(row[3])
        blocks_group = int(row[5])
        inodes_group = int(row[6])
super.close()

#group
############################################################################
iBITMAP = [] #stays as hex
bBITMAP = [] #stays as hex

with open("group.csv", "r") as group:
    groupfile = csv.reader(group)
    for row in groupfile:
        iBITMAP.append(row[4])
        bBITMAP.append(row[5])
group.close()



#bitmap
############################################################################
iFREE = [] #actual ints
bFREE = [] #actual ints

with open("bitmap.csv", "r") as bitmap:
    bitmapfile = csv.reader(bitmap)
    for row in bitmapfile:
        if row[0] in iBITMAP:
            iFREE.append(int(row[1]))
        if row[0] in bBITMAP:
            bFREE.append(int(row[1]))
bitmap.close()

#indirect table dictionary
############################################################################
intable = {} #indirect is hex string - entry is dec str, pointer is int
with open("indirect.csv", "r") as indirect:
    indirectfile = csv.reader(indirect)
    for row in indirectfile:
        intable[row[0]+"-"+row[1]] = int(row[2],16)
indirect.close()


#inode
############################################################################
aINODES = {} #int, class
aBLOCKS = {} #int, class

class inode:
    def __init__(self, number, links): #all are actual ints
        self.number = number
        self.links = links
        self.ref = []
        self.pointers = []
  
class block:
    def __init__(self, number): 
        self.number = number    #int
        self.ref = []           #list of strings "inodenumber,indirect,entry"





def iter_indir(bnum, inumber, recurs): #, indir, iblock):  #bnum is int, inumber is str int,entry is int,recurs is int
    if(recurs == -1):
        return
    recurs -= 1
    for key in intable:
        blockn, entryn = key.split("-")
        blockn = int(blockn, 16)
        block_ptr = intable[key]
        if (blockn == bnum):
            if(block_ptr in aBLOCKS):
                aBLOCKS[block_ptr].ref.append(inumber+","+str(bnum)+","+entryn)
            else:
                aBLOCKS[block_ptr] = block(block_ptr)
                aBLOCKS[block_ptr].ref.append(inumber+str(bnum)+entryn)
            aINODES[int(inumber)].pointers.append(block_ptr)
            iter_indir(block_ptr,inumber,recurs)
        
            #print "INVALIDBLOCK < "+str(bnum)+" > IN INODE < "+inumber+" > INDIRECT BLOCK < "+str(iblock)+" > ENTRY < "+str(entry)+" >"
    


            
ptperblock = blocksize/4
indirectblock = ptperblock+12
doublely = (ptperblock*ptperblock)+indirectblock
triplely = (ptperblock*ptperblock*ptperblock)+doublely

count = 0

with open("inode.csv", "r") as inodef:
    inodefile = csv.reader(inodef)
    for row in inodefile:
        inumber = int(row[0]) #inode number, int
        links = int(row[5])   #link number of inode, int
        aINODES[inumber] = inode(inumber, links)
        r = int(row[10])
        
        if(r > 12):
            if(r < indirectblock):
                r = 13
            elif(r < doublely):
                r = 14
            elif(r < triplely):
                r = 15
        
        for i in range(r):
            bnum = int(row[i+11], 16)
            if(i < 12):
                if(bnum == 0 or bnum > nBLOCKS):
                    print "INVALIDBLOCK < "+str(bnum)+" > IN INODE < "+row[0]+" > ENTRY < "+str(i)+" >"
                    continue
                if(bnum in aBLOCKS):
                    aBLOCKS[bnum].ref.append(row[0]+",0,"+str(i))
                else:
                    aBLOCKS[bnum] = block(bnum)
                    aBLOCKS[bnum].ref.append(row[0]+",0,"+str(i))
                aINODES[inumber].pointers.append(bnum)
            if(i >= 12):
                if(i == 12):
                    recurs = 0
                if(i == 13):
                    recurs = 1
                if(i == 14):
                    recurs = 2

                check = 0
                for key in intable:
                    blockn, entryn = key.split("-")
                    blockn = int(blockn, 16)
                    if bnum == blockn:
                        check = 1
                        if(bnum in aBLOCKS):
                            aBLOCKS[bnum].ref.append(row[0]+",0,"+str(i))
                        else:
                            aBLOCKS[bnum] = block(bnum)
                            aBLOCKS[bnum].ref.append(row[0]+",0,"+str(i))
                        aINODES[inumber].pointers.append(bnum)
                        break
                if check == 0:
                    errorprint += "INVALIDBLOCK < "+str(bnum)+" > IN INODE < "+row[0]+" > ENTRY < "+str(i)+" >"+'\n'
                iter_indir(bnum,row[0],recurs)
        
inodef.close()

#unallocated block
##################################################
for key in aBLOCKS:
    if key in bFREE:
        errorout = ""
        for x in aBLOCKS[key].ref:
            sinode, sindirect, sentry = x.split(",")
            if sindirect == "0":
                errorout = errorout + " INODE < "+sinode+" > ENTRY < "+sentry+" >"
            else:
                errorout = errorout + " INODE < "+sinode+" > INDIRECT BLOCK < "+sindirect+" > ENTRY < "+sentry+" >"
        errorprint += "UNALLOCATED BLOCK < "+str(key)+" > REFERENCED BY"+errorout+'\n'
#multiply reference block
##################################################
for key in aBLOCKS:
    if len(aBLOCKS[key].ref) > 1:
        errorout = ""
        for x in aBLOCKS[key].ref:
            sinode, sindirect, sentry = x.split(",")
            if sindirect == "0":
                errorout = errorout + " INODE < "+sinode+" > ENTRY < "+sentry+" >"
            else:
                errorout = errorout + " INODE < "+sinode+" > INDIRECT BLOCK < "+sindirect+" > ENTRY < "+sentry+" >"
        errorprint += "MULTIPLY REFERENCED BLOCK < "+str(key)+" > BY"+errorout+'\n'

#missing inode
##################################################
for key in aINODES:
    if aINODES[key].links == 0 and key > 10:
        index = (key/inodes_group)
        free_list = int(iBITMAP[index],16)
        errorprint += "MISSING INODE < "+str(key)+" > SHOULD BE IN FREE LIST < "+str(free_list)+" >"+'\n'

#directory
##################################################
direct_table = {} #both strings

unalloc_inode = {} #inode string: "direcotry-inode,entry"

with open("directory.csv", "r") as directory:
    directoryfile = csv.reader(directory)
    for row in directoryfile:
        if(int(row[1]) == 0):
            n_inode = int(row[4])
            aINODES[n_inode].ref.append(int(row[0]))
            if row[4] != row[0]:
                errorprint += "INCORRECT ENTRY IN < "+row[0]+" > NAME < . > LINK TO < "+row[4]+" > SHOULD BE < "+row[0]+" >"+'\n'
            continue
        if(int(row[1]) == 1):
            n_inode = int(row[4])
            aINODES[n_inode].ref.append(int(row[0]))
            if (row[0] == 2 and row[4] != 2):
                errorprint += "INCORRECT ENTRY IN < 2 > NAME < .. > LINK TO < "+row[4]+" > SHOULD BE < 2 >"+'\n'
            elif int(row[0]) != 2 and direct_table[row[0]] != row[4]:
                errorprint += "INCORRECT ENTRY IN < "+row[0]+" > NAME < .. > LINK TO < "+row[4]+" > SHOULD BE < "+direct_table[row[0]]+" >"+'\n'
            continue
        direct_table[row[4]] = row[0]
        if not int(row[4]) in aINODES:
            if int(row[4]) in unalloc_inode:
                unalloc_inode[int(row[4])].append(row[0]+","+row[1])
            else:
                unalloc_inode[int(row[4])] = [row[0]+","+row[1]]
        else:
            n_inode = int(row[4])
            aINODES[n_inode].ref.append(int(row[0]))
directory.close()

#unallocated inode
#####################################################
for i in unalloc_inode:
    errorout = ""
    for x in unalloc_inode[i]:
        direc, entry = x.split(",")
        errorout = errorout + " REFERENCED BY DIRECTORY < "+direc+" > ENTRY < "+entry+" >"
    errorprint += "UNALLOCATED INODE < "+str(i)+" >"+errorout+'\n'

#link count check
#####################################################
for i in aINODES:
    if len(aINODES[i].ref) != aINODES[i].links:
        errorprint += "LINKCOUNT < "+str(i)+" > IS < "+str(aINODES[i].links)+" > SHOULD BE < "+str(len(aINODES[i].ref))+" >"+'\n'


checkfd = open('lab3b_check.txt', 'w')

checkfd.write(errorprint)
checkfd.close()
