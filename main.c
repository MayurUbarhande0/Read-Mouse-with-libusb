
 #include <libusb-1.0/libusb.h> 
#include <stdint.h>
#include <stdio.h> 
#include <sys/types.h>
void Mylooger(libusb_context* ctx , enum libusb_log_level level , const char* msg )  {
    (void) ctx  ;
    (void) level ;
    printf("%s\n" , msg) ;
  }
const struct libusb_endpoint_descriptor*  get_endpoint(struct libusb_device* ld , struct libusb_config_descriptor** desc , int interface_number ){ 
  // there exits a hierarchy , which we have to follow to get to endpoint of the device 
  //first there is active config descriptor struct in that there is member which the value number of interfaces a device as 
  // and list of the interfaces and there are many more member such as maxpower , blength ,etc . 
  // It flow as config_desc -> interfac -> interface_desc -> endpoint 
   if(libusb_get_active_config_descriptor(ld, &(*desc))  < 0){
     
     return NULL ;  
    }

  
   if((*desc)->bNumInterfaces>0){   
    // here we are checking if the number of interfaces are greater then one
     const struct libusb_interface* inter   = &(*desc)->interface[interface_number ]  ;  
     // we are created a interface struct that holds the interface 
     if(inter->num_altsetting > 0){
        const struct libusb_interface_descriptor* idesc  = &inter->altsetting[0] ; 
        // in the interface struct there is a member "altsetting" that gives the interface descriptor 
        if(idesc->bNumEndpoints>0){
          return &idesc->endpoint[0] ;  
          // here we get the interface descriptor
        }
     }
     
    
   }
    libusb_free_config_descriptor(*desc) ;
    // we should free the config descriptor
    //and mark the address as NUll 
     *desc = NULL ; 
     return NULL  ; 
}
int main(){
  //start setup ctx and loggers
    libusb_context *ctx ;
  
    libusb_log_cb cb = Mylooger  ;
  
  // init libusb using ctx
   int res =  libusb_init(&ctx);
    if(res != LIBUSB_SUCCESS ){
        printf(" not success") ;
        return 1 ;
    }
    
    
   //set up sb 
    libusb_set_log_cb(ctx, cb  ,  4); 
    libusb_device **  ld  ;  //double pointer stores list of device 
    libusb_device *found = NULL ;  // if foudn target device store here
    ssize_t cn = libusb_get_device_list(ctx, &ld) ; // get list of target devices args as ctx and triple pointer to ld  

    printf("number of device is %zi \n  " , cn) ;  

    if(cn<0){
      printf("err while device list") ;
      libusb_exit(ctx) ;
      return 1 ;
    }  
    uint16_t target_v = 0x1c4f ; //target device vendor id
    uint16_t target_p = 0x0048 ; // target device product id 
      
    struct  libusb_device_descriptor des  ;  // device descriptor struct it stores the vendor and product id of device 
     // it also hold many details about device other then the endpoint  

    for(ssize_t i = 0 ; i<cn ; i++) {
       int res = libusb_get_device_descriptor(ld[i], &des) ;  // geting device description one by ine 
       if(res !=0 ){
        cb(ctx,3 , libusb_error_name(res)) ;
        
       }  
       
      if(des.idVendor == target_v && des.idProduct == target_p ){
         //matching it with our desired target 
       
        found =  ld[i];  
        break ;
      }
    
        } 
    if(found == NULL){ 

      //if not found exit
        printf("no such device\n") ;
        libusb_exit(ctx) ;
        return 1 ;
    }
    libusb_device_handle* handler ;  //it is and handler struct it is of opque type not access by us we just pass it by pointer  
    int err = libusb_open(found, &handler) ; // we open the device  it gives err that is subsequently checked by switch case 
    
    switch (err) {
     case 0 :
           printf("successfull") ;
           break; 
     case LIBUSB_ERROR_NO_MEM :
          printf("no mem") ;

          break; 
     case LIBUSB_ERROR_ACCESS :
          printf("not root") ;
          break;
     case  LIBUSB_ERROR_NO_DEVICE :
          printf("no such device") ;
          break;

          default: 
       printf("err is %s \n" , libusb_error_name(err)) ;
    }
    if(err!=0){
       libusb_exit(ctx) ;
       return 1 ;
    }

  err = libusb_set_auto_detach_kernel_driver(handler, 1) ; // here we check if the kernel hold the device driver 
  // because it may cause err/panic in the kernel so we detach it there are two func we can use one is auto and one is manual
  if(err<0){
    printf("err detaching kernal\n" ) ;
    libusb_close(handler) ;
    libusb_exit(ctx) ;
    return 1 ;
  }
  struct libusb_config_descriptor* config_desc = NULL ;
  const struct libusb_endpoint_descriptor* edesc  = get_endpoint(found , &config_desc , 0) ;
  if(edesc  == NULL){
    printf("err getting endpoint") ;
    libusb_close(handler ) ;
    libusb_exit(ctx) ;
    return 1  ; 
  }
  uint8_t endpoint = edesc->bEndpointAddress ;
  uint16_t maxsize = edesc->wMaxPacketSize ;
   err = libusb_claim_interface(handler, 0) ; //to perform any IO operation we must claim the interface
   // and when our work is done we should free it  
       libusb_free_device_list(ld, 1) ;
   if(err<0){
    printf("err claiming interface \n") ;
    libusb_close(handler) ;
     libusb_exit(ctx) ;
     return 1  ;
  }
   unsigned char data[maxsize] ; 
   int actuallen = 0 ; 
   int count = 0  ;
   while(count < 21 ){
    int err  = libusb_interrupt_transfer(handler ,endpoint ,data , maxsize , &actuallen , 500) ;
    if(err == 0 && actuallen > 0 ){
      printf("bytes size is %d" , actuallen ) ;
      for(int i = 0 ; i<actuallen ; i++){
        printf("%02x\n" , data[i]) ;
      }
      count ++ ;
    }
   }

    
  libusb_release_interface(handler, 0) ;
   libusb_attach_kernel_driver(handler, 0) ;
  libusb_close(handler) ;
   
    

  libusb_exit(ctx) ;
  return   0 ;

   
}