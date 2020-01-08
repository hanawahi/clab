  ////////////////////////////////////////////////////////////////////////////////
  //
  //  File           : cart_driver.c
  //  Description    : This is the implementation of the standardized IO functions
  //                   for used to access the CART storage system.
  //
  //  Author         : Hana Wahi-Anwar
  //  PSU email      : HQW5245@psu.edu 
  //

  // Includes
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>

  // Project Includes
  #include "cart_driver.h"
  #include "cart_controller.h"

  /*   MY DATA STRUCTURE:     */
  /*  Variable Descriptions.        ////
   *  The following variables are the variables within the
   *  struct FileT; each individual file had the following
   *  unique variables:
   *  fileSize - kept track of file size
   *  filePos - kept track of current position in file
   *  framePos - latest position in frame in file
   *  fileName - stored name of file
   *  framesUsed - array/list of frames used for the file
   *  frames - current number of frames used for the file
   *  status - if file was open: 1 = open, 0 = closed
   *  Global Variable Descriptions. ////
   *  numFiles - array of structs that holds all files
   *  frameCounter - counts total frames used (by all files)
   *  frameIndex - holds current frame
   *  cartridgeIndex - holds current cartridge
   */
  typedef struct File {
    int fileSize;
    int filePos;
    int framePos;
    char fileName [CART_MAX_PATH_LENGTH];
    int framesUsed [100];
    int frames;
    int status;
  }FileT;
  /* Globals: */
  static FileT numFiles [CART_MAX_TOTAL_FILES];
  static int frameCounter = 0;
  static CartFrameIndex frameIndex;
  static CartridgeIndex cartridgeIndex;
  /* END DATA STRUCTURE */



  //
  // Implementation


  //////////////////////////////////////////
  //
  // Function     : Cart_io_bus
  // Description  : Wrapper function checks for error.
  //
  // Inputs       : opcode, *buf
  // Outputs      : 0 if successful, -1 if error.

  int Cart_io_bus(CartXferRegister opcode, void *buf){
    CartXferRegister state = cart_io_bus(opcode, buf);
    if ((((state << 16) >> 63) & 1) != 0){
      return -1;
      }
    else
      return 0;
}

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : getOpCode
  // Description  : Converts to usable opcode.
  //
  // Inputs       : KY1, KY2, RT1, CT1, FM1
  // Outputs      : opcode if successful

  CartXferRegister getOpcode(CartXferRegister KY1, CartXferRegister KY2, CartXferRegister RT1,
                                      CartXferRegister CT1, CartXferRegister FM1){

    return ((KY1 << 56) | (KY2 << 48) | (RT1 << 47) | (CT1 << 31) | (FM1 << 15));
}

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : cart_poweron
  // Description  : Startup up the CART interface, initialize filesystem
  //
  // Inputs       : none
  // Outputs      : 0 if successful, -1 if failure

  int32_t cart_poweron(void) {
    CartXferRegister state;
    int i;

  /* CART_OP_INITMS: initialize the interface: */  
    if ((state = Cart_io_bus(getOpcode(CART_OP_INITMS, 0, 0, 0, 0), NULL)) == -1){
      return -1;
    }

    for (i = 0; i < CART_MAX_CARTRIDGES; i++){
      /* Load a cartridge: */ 
      if ((state = Cart_io_bus(getOpcode(CART_OP_LDCART, 0, 0, i, 0), NULL)) == -1) {
      return -1;
      }
    /* Zero the currently loaded cartridge: */
    if ((state = Cart_io_bus(getOpcode(CART_OP_BZERO, 0, 0, 0, 0), NULL)) == -1) {
      return -1;
      }
    }

    for (i = 0; i < CART_MAX_TOTAL_FILES; i++) {

      numFiles[i].fileSize = 0;
      numFiles[i].filePos = 0;
      numFiles[i].frames = 0;
      strcpy(numFiles[i].fileName,  "");
     /* Initialize framesUsed: */
      int j;
      for (j = 0; j < 100; j++){
        numFiles[i].framesUsed[j] = -1;
      }
      numFiles[i].framePos = 0;
      numFiles[i].status = 1;
    }
    // Return successfully
    return(0);
}

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : cart_poweroff
  // Description  : Shut down the CART interface, close all files
  //
  // Inputs       : none
  // Outputs      : 0 if successful, -1 if failure

  int32_t cart_poweroff(void) {
    CartXferRegister state;
    int i;
    for(i=0; i < CART_MAX_CARTRIDGES; i++){
      if((state = Cart_io_bus(getOpcode(CART_OP_LDCART, 0, 0, i, 0), NULL)) == -1){
        return -1;
      }
      if((state = Cart_io_bus(getOpcode(CART_OP_BZERO, 0, 0, 0, 0), NULL)) == -1) {
        return -1;
      }
    }
    if((state = Cart_io_bus(getOpcode(CART_OP_POWOFF, 0, 0, 0, 0), NULL)) == -1) {
      return -1;
    }
    for (i = 0; i < CART_MAX_TOTAL_FILES; i++) {
      numFiles[i].fileSize = 0;
      numFiles[i].filePos = 0;
      numFiles[i].frames = 0;
      strcpy(numFiles[i].fileName, "");

     // int j;
     // for (j = 0; j < 100; j++){
     //   numFiles[i].framesUsed[j] = -1;
     // }
      numFiles[i].framePos = 0;
      numFiles[i].status = 0;
    }
    frameIndex = 0;
    cartridgeIndex = 0;
    //free(numFiles); 
    
    
    
    // Return successfully
    return(0);
}

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : cart_open
  // Description  : This function opens the file and returns a file handle
  //
  // Inputs       : path - filename of the file to open
  // Outputs      : file handle if successful, -1 if failure

  int16_t cart_open(char *path) {

    int i;
    for (i = 0; i < CART_MAX_TOTAL_FILES; i++){
    /* If names match: */
      if (strcmp(path, numFiles[i].fileName) == 0) {
        /* If names match, set status to on (1). */
        numFiles[i].status = 1;
        numFiles[i].filePos = 0;
        return i;
      }

    }
    /* If end of files, and no file is found, make new file: */
    /* Check for next empty file to create new file into: */
    for (i = 0; i < CART_MAX_TOTAL_FILES; i++){
        if (strcmp(numFiles[i].fileName, "") == 0){
          /* Make new file: */
          numFiles[i].fileSize = 0;
          numFiles[i].filePos = 0;
          numFiles[i].frames = 0;
          strcpy(numFiles[i].fileName, path);
          /* Initialize framesUsed: */
          int j;
          for (j = 0; j<100; j++){
            numFiles[i].framesUsed[j] = -1;
          }
          numFiles[i].status = 1;
        return i;
        }
    }

    // THIS SHOULD RETURN A FILE HANDLE
    return (i);
}

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : cart_close
  // Description  : This function closes the file
  //
  // Inputs       : fd - the file descriptor
  // Outputs      : 0 if successful, -1 if failure

  int16_t cart_close(int16_t fd) {
    /* Error Check: */
    /* If file is already closed: */
    if (numFiles[fd].status == 0) {
      return -1;
    }
    /* If file handle is bad: */
    if ((fd < 0) || (fd > (CART_FRAME_SIZE-1))) {
      return -1;
    }

    /* Close file. */
    numFiles[fd].status = 0;


    // Return successfully
    return (0);
}

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : cart_read
  // Description  : Reads "count" bytes from the file handle "fh" into the 
  //                buffer "buf"
  //
  // Inputs       : fd - filename of the file to read from
  //                buf - pointer to buffer to read into
  //                count - number of bytes to read
  // Outputs      : bytes read if successful, -1 if failure

  int32_t cart_read(int16_t fd, void *buf, int32_t count) {
    /*Error Check: */
    /* If file is closed, cannot write (this is an error) */
    if (numFiles[fd].status == 0) {
      return -1;
    }

    /* Check length to see if valid: */
    if (count < 0) {
      return -1;

    }
    
    /* Check if file is out of bounds: */
    if ((fd < 0) || (fd > (CART_FRAME_SIZE-1))) {
       return -1;
    }
    
    /* ////// ERROR CHECK END ////// */


    /* Reset count if needed: */
    if (count > (numFiles[fd].fileSize - numFiles[fd].filePos)) {
      count = numFiles[fd].fileSize - numFiles[fd].filePos;
    }
    
    /* These are temporary buffers: */
    char buffer [CART_FRAME_SIZE];
    char largeBuffer [CART_FRAME_SIZE * CART_CARTRIDGE_SIZE];
    /* Get frame and cartridge indices: */
    frameIndex = (numFiles[fd].filePos/CART_FRAME_SIZE) + numFiles[fd].framesUsed[0]; 
    cartridgeIndex = frameIndex / CART_CARTRIDGE_SIZE; 
    numFiles[fd].framePos = numFiles[fd].filePos % CART_FRAME_SIZE;
    /* Keep track of bytes read: */
    int  bytesRead = 0;
    
    /* current frame index in framesUsed[]: */
    int i;
    for (i = 0; i < 100; i++) {
      if (numFiles[fd].framesUsed[i] == frameIndex) {
        break;
      }
    }

    

    /* Begin loop: */
    while (count > 0) {
    
      /* Load cart: */
      if ((Cart_io_bus(getOpcode(CART_OP_LDCART, 0, 0, cartridgeIndex, 0), NULL)) == -1){
        return -1;
      }

      /* Read frame: */
      if ((Cart_io_bus(getOpcode(CART_OP_RDFRME, 0, 0, 0, frameIndex), buffer)) == -1){
        return -1;
      }
      
      /* If count exceeds frame size: */
      if(count > CART_FRAME_SIZE-numFiles[fd].framePos) {
        memcpy(&largeBuffer[bytesRead], &buffer[numFiles[fd].framePos], (CART_FRAME_SIZE - numFiles[fd].framePos));
        /* Update bytesRead and count: */
        bytesRead = bytesRead + (CART_FRAME_SIZE-numFiles[fd].framePos);
        count = count - (CART_FRAME_SIZE-numFiles[fd].framePos);
      }
      
      /* If count does not exceed frame size: */
      else {
        memcpy(&largeBuffer[bytesRead], &buffer[numFiles[fd].framePos], count);
        /* Update bytesRead and count: */
        bytesRead = bytesRead + count;
        count = count - bytesRead;
      
      }    
      
      numFiles[fd].filePos = numFiles[fd].filePos + bytesRead;
      i++;
      frameIndex = numFiles[fd].framesUsed[i];
      cartridgeIndex = frameIndex / CART_CARTRIDGE_SIZE; 
      numFiles[fd].framePos = numFiles[fd].filePos%CART_FRAME_SIZE;
    }
    /* While loop ends here */
    
    
    
    memcpy(buf, largeBuffer, bytesRead);


    // Return successfully
    return (bytesRead);
}

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : cart_write
  // Description  : Writes "count" bytes to the file handle "fh" from the 
  //                buffer  "buf"
  //
  // Inputs       : fd - filename of the file to write to
  //                buf - pointer to buffer to write from
  //                count - number of bytes to write
  // Outputs      : bytes written if successful, -1 if failure

  int32_t cart_write(int16_t fd, void *buf, int32_t count) {
    /*Error Check: */
    /* If file is closed, cannot write (this is an error) */
    if (numFiles[fd].status == 0) {
      return -1;
    }
    
    /* Check length to see if valid: */
    if (count < 0) {
      return -1;

    }
    
    /* Check if file is out of bounds: */
    if ((fd < 0) || (fd > (CART_FRAME_SIZE-1))) {
       return -1;
    }
    /* ////// ERROR CHECK END ////// */
    
    

    /* These are temp buffers */
    char buffer [CART_FRAME_SIZE];
    char largeBuffer [CART_FRAME_SIZE * CART_CARTRIDGE_SIZE];
    /* Keeps track of if allocation is needed: */ 
    int needFrame = 1;

    /* Check if allocation is needed: */
    int i;
    for (i = 0; i < 100; i++) {
      if (numFiles[fd].framesUsed[i] == frameIndex) {
      /* Allocation not needed */
        needFrame = 0;
        break;
      }
    }
      
    if (needFrame != 0) {
        numFiles[fd].framesUsed[numFiles[fd].frames] = frameCounter;
        numFiles[fd].frames++;
        frameCounter++;
    }
    
    /* Get frame and cartridge indices: */
    frameIndex = (numFiles[fd].filePos/CART_FRAME_SIZE) + numFiles[fd].framesUsed[0]; 
    cartridgeIndex = frameIndex / CART_CARTRIDGE_SIZE; 
    numFiles[fd].framePos = numFiles[fd].filePos%CART_FRAME_SIZE;
    /* Bytes written so far: */
    int bytesWrote = 0;

    /* Load cartridge: */
    if ((Cart_io_bus(getOpcode(CART_OP_LDCART, 0, 0, cartridgeIndex, 0), NULL)) == -1) {
      return -1;
    }
    
    memcpy(&largeBuffer, (char *) buf, count);
    
    /* Loop begins */
    while (count > 0) {
      /* Check if allocation is needed: */
      int i;
      for (i = 0; i < 100; i++) {
        if (numFiles[fd].framesUsed[i] == frameIndex) {
        /* Allocation not needed */
          needFrame = 0;
          break;
        }
      }
      /* If allocation is needed: */
      if (needFrame != 0) {
        numFiles[fd].framesUsed[numFiles[fd].frames] = frameCounter;
        numFiles[fd].frames++;
        frameCounter++;
      }
    
      /* Read frame: */
      if ((Cart_io_bus(getOpcode(CART_OP_RDFRME, 0, 0, 0, frameIndex), buffer)) == -1) {
        return -1;
      }
      
      /* If count exceeds frame size: */
      if(count > CART_FRAME_SIZE-numFiles[fd].framePos) {
        memcpy(&buffer[numFiles[fd].framePos], &largeBuffer[bytesWrote], CART_FRAME_SIZE-numFiles[fd].framePos);
        /* Update bytesWrote and count: */
        bytesWrote = bytesWrote + (CART_FRAME_SIZE-numFiles[fd].framePos);
        count = count - (CART_FRAME_SIZE-numFiles[fd].framePos);
      }
      /* If count does not exceed frame size: */
      else {
        memcpy(&buffer[numFiles[fd].framePos], &largeBuffer[bytesWrote], count);
        /* Update bytesWrote and count: */
        bytesWrote = bytesWrote + count;
        count = count - bytesWrote;
      }
      
      /* Write frame: */
      if ((Cart_io_bus(getOpcode(CART_OP_WRFRME, 0, 0, 0, frameIndex), buffer)) == -1) {
        return -1;
      }
      
      /* Check if next cartridge: */
      if((frameIndex + 1) == CART_CARTRIDGE_SIZE) {
        if((cartridgeIndex + 1) == CART_MAX_CARTRIDGES) {
          return -1;
        }
        cartridgeIndex++;
        frameIndex = 0; 
        /* Load cartridge: */
        if ((Cart_io_bus(getOpcode(CART_OP_LDCART, 0, 0, cartridgeIndex, 0), NULL)) == -1) {
          return -1;
        }
      }
      /* If next cartridge not needed: */
      else {
      /* go to next frame. */
      frameIndex++;
      }
      numFiles[fd].framePos = 0;
      needFrame = 1;
    } 
    /* While loop ends here */
        

    /* Update file position and file size: */
    numFiles[fd].filePos+=bytesWrote;
    numFiles[fd].fileSize+=bytesWrote;

    // Return successfully
    return (bytesWrote);
}


  ////////////////////////////////////////////////////////////////////////////////
  //
  // Function     : cart_seek
  // Description  : Seek to specific point in the file
  //
  // Inputs       : fd - filename of the file to write to
  //                loc - offfset of file in relation to beginning of file
  // Outputs      : 0 if successful, -1 if failure

  int32_t cart_seek(int16_t fd, uint32_t loc) {
    /* Error check: */
    /* Attempting to access an index that does not exist: */
    if ((fd < 0) || (fd > (CART_FRAME_SIZE-1))) {
       return -1;
    }

    /* Attempting to access a closed file: */
    if (numFiles[fd].status == 0) {
      return -1;
    }

    /* Attempting to access a location larger than filesize: */
    if (loc > numFiles[fd].fileSize) {
      return -1;
    }

    /* If filename is "": */
    if (strcmp(numFiles[fd].fileName, "") == 0) {
      return -1;
    }

    /* If size < 0: */
    if (numFiles[fd].fileSize < 0) {
      return -1;
    }  
    /* ////// ERROR CHECK END ////// */
   
    /* If passes all of the above cases, proceed: */
    numFiles[fd].filePos = loc;


    // Return successfully
    return (0);
}
