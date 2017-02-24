#include <BLE_API.h>
BLE           ble;

static uint8_t led_found = 0; // have we found the YN360 device yet?
static uint8_t characteristic_is_found = 0; // does this device have the characteristic we want to communicate with?

// To save the hrm characteristic and descriptor
static DiscoveredCharacteristic            chars_led; // to store the address of the characteristic we want to communicate with, easier than storing whole multi-byte uuid of characteristic

static void scanCallBack(const Gap::AdvertisementCallbackParams_t *params);
static void discoveredServiceCallBack(const DiscoveredService *service);
static void discoveredCharacteristicCallBack(const DiscoveredCharacteristic *chars);
static void discoveryTerminationCallBack(Gap::Handle_t connectionHandle);
static void discoveredCharsDescriptorCallBack(const CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t *params);
static void discoveredDescTerminationCallBack(const CharacteristicDescriptorDiscovery::TerminationCallbackParams_t *params) ;

static uint8_t valueArray[6] = {0xAE,0xEE,0x00,0x00,0x00,0x56}; // initial data to send - code for all lights off
int lightMode = 0;
int colourAngle = 0;
int redLed = 0;
int greenLed = 0;
int blueLed = 0;
int brightness = 10; //less blinding for development - 255 for full power;
uint32_t ble_advdata_parser(uint8_t type, uint8_t advdata_len, uint8_t *p_advdata, uint8_t *len, uint8_t *p_field_data) {
  uint8_t index=0;
  uint8_t field_length, field_type;
  while(index<advdata_len) {
    field_length = p_advdata[index];
    field_type   = p_advdata[index+1];
    if(field_type == type) {
      memcpy(p_field_data, &p_advdata[index+2], (field_length-1));
      *len = field_length - 1;
      return NRF_SUCCESS;
    }
    index += field_length + 1;
  }
  return NRF_ERROR_NOT_FOUND;
}

void startDiscovery(uint16_t handle) {
  /**
   * Launch service discovery. Once launched, application callbacks will beinvoked for matching services or characteristics.
   * isServiceDiscoveryActive() can be used to determine status, and a termination callback (if one was set up)will be invoked at the end.
   * Service discovery can be terminated prematurely,if needed, using terminateServiceDiscovery().
   *
   * @param[in]  connectionHandle   Handle for the connection with the peer.
   * @param[in]  sc  This is the application callback for a matching service. Taken as NULL by default.
   *                 Note: service discovery may still be active when this callback is issued;
   *                 calling asynchronous BLE-stack APIs from within this application callback might cause the stack to abort service discovery.
   *                 If this becomes an issue, it may be better to make a local copy of the discoveredService and wait for service discovery to terminate before operating on the service.
   * @param[in]  cc  This is the application callback for a matching characteristic.Taken as NULL by default.
   *                 Note: service discovery may still be active when this callback is issued;
   *                 calling asynchronous BLE-stack APIs from within this application callback might cause the stack to abort service discovery.
   *                 If this becomes an issue, it may be better to make a local copy of the discoveredCharacteristic and wait for service discovery to terminate before operating on the characteristic.
   * @param[in]  matchingServiceUUID  UUID-based filter for specifying a service in which the application is interested.
   *                                  By default it is set as the wildcard UUID_UNKNOWN, in which case it matches all services.
   * @param[in]  matchingCharacteristicUUIDIn  UUID-based filter for specifying characteristic in which the application is interested.
   *                                           By default it is set as the wildcard UUID_UKNOWN to match against any characteristic.
   *
   * @note     Using wildcard values for both service-UUID and characteristic-UUID will result in complete service discovery:
   *           callbacks being called for every service and characteristic.
   *
   * @note     Providing NULL for the characteristic callback will result in characteristic discovery being skipped for each matching service.
   *           This allows for an inexpensive method to discover only services.
   *
   * @return   BLE_ERROR_NONE if service discovery is launched successfully; else an appropriate error.
   */
    Serial.print("\n3. Device Services\n");
    ble.gattClient().launchServiceDiscovery(handle, discoveredServiceCallBack, discoveredCharacteristicCallBack);

}


//
//void setParams(const Gap::AdvertisementCallbackParams_t *params) {
//
//
//
//ble.gap().setPreferredConnectionParams(params)

//
//uint16_t   minConnectionInterval
// 
//uint16_t  maxConnectionInterval
// 
//uint16_t  slaveLatency
// 
//uint16_t  connectionSupervisionTimeout

/**
 * @brief  Callback handle for scanning device
 *
 * @param[in]  *params   params->peerAddr            The peer's BLE address
 *                       params->rssi                The advertisement packet RSSI value
 *                       params->isScanResponse      Whether this packet is the response to a scan request
 *                       params->type                The type of advertisement
 *                                                   (enum from 0 ADV_CONNECTABLE_UNDIRECTED,ADV_CONNECTABLE_DIRECTED,ADV_SCANNABLE_UNDIRECTED,ADV_NON_CONNECTABLE_UNDIRECTED)
 *                       params->advertisingDataLen  Length of the advertisement data
 *                       params->advertisingData     Pointer to the advertisement packet's data
 */
 

void scanCallBack(const Gap::AdvertisementCallbackParams_t *params) {
 uint8_t index;
 uint8_t len;
 uint8_t adv_name[31];

 if( NRF_SUCCESS == ble_advdata_parser(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, params->advertisingDataLen, (uint8_t *)params->advertisingData, &len, adv_name) ) {
    Serial.print(" - ");
      for(index=0; index< len; index++) {
        Serial.print((char)adv_name[index]);
      }
   } else if( NRF_SUCCESS == ble_advdata_parser(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, params->advertisingDataLen, (uint8_t *)params->advertisingData, &len, adv_name) ) {
    Serial.print(" - ");
      for(index=0; index< len; index++) {
        Serial.print((char)adv_name[index]);
      }
   }
   else {Serial.print(" - Unknown Device"); }

    
  Serial.print(" found at ");
  for(index=0; index<6; index++) {
    Serial.print(params->peerAddr[index], HEX);
  }
  Serial.print(", RSSI: ");
  Serial.print(params->rssi, DEC);
  Serial.println(" ");

  if( NRF_SUCCESS == ble_advdata_parser(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, params->advertisingDataLen, (uint8_t *)params->advertisingData, &len, adv_name) ) {
    if(memcmp("YONGNUO LED", adv_name, 11) == 0x00) {
          Serial.println(" * Found target device, stopping scan ");
          ble.stopScan();
          led_found = 1;
         Gap::ConnectionParams_t *connectionParams; 
          connectionParams->minConnectionInterval = uint16_t(1);
          connectionParams->maxConnectionInterval = uint16_t(1000);
          connectionParams->slaveLatency = uint16_t(0);
          connectionParams->connectionSupervisionTimeout = uint16_t(4000);
          
          ble.connect(params->peerAddr, BLEProtocol::AddressType::RANDOM_STATIC, connectionParams, NULL);
    }
  }
}

/** @brief  Connection callback handle
 *
 *  @param[in] *params   params->handle : The ID for this connection
 *                       params->role : PERIPHERAL  = 0x1, // Peripheral Role
 *                                      CENTRAL     = 0x2, // Central Role.
 *                       params->peerAddrType : The peer's BLE address type
 *                       params->peerAddr : The peer's BLE address
 *                       params->ownAddrType : This device's BLE address type
 *                       params->ownAddr : This devices's BLE address
 *                       params->connectionParams->minConnectionInterval
 *                       params->connectionParams->maxConnectionInterval
 *                       params->connectionParams->slaveLatency
 *                       params->connectionParams->connectionSupervisionTimeout
 */
void connectionCallBack( const Gap::ConnectionCallbackParams_t *params ) {
  uint8_t index;

  Serial.print("2. Connected to ");
  for(index=0; index<6; index++) {Serial.print(params->peerAddr[index], HEX);}
  Serial.print(", connection ID:");
  Serial.println(params->handle, HEX);

  // start discovering services on the device we have connected to
  startDiscovery(params->handle);
}


/** @brief  Disconnect callback handle
 *
 *  @param[in] *params   params->handle : connect handle
 *                       params->reason : CONNECTION_TIMEOUT                          = 0x08 = 8,
 *                                        REMOTE_USER_TERMINATED_CONNECTION           = 0x13 = 19,
 *                                        REMOTE_DEV_TERMINATION_DUE_TO_LOW_RESOURCES = 0x14 = 20,  // Remote device terminated connection due to low resources.
 *                                        REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF     = 0x15 = 21,  // Remote device terminated connection due to power off.
 *                                        LOCAL_HOST_TERMINATED_CONNECTION            = 0x16 = 22,
 *                                        CONN_INTERVAL_UNACCEPTABLE                  = 0x3B = 59,
 */
void disconnectionCallBack(const Gap::DisconnectionCallbackParams_t *params) {
  Serial.print("Connection Disconnected.\n");
  Serial.print("Reason: ");

  switch (params->reason) {
    case 8:
      Serial.println("CONNECTION_TIMEOUT");
      break;
    case 19:
      Serial.println("REMOTE_USER_TERMINATED_CONNECTION");
      break;
    case 20:
      Serial.println("REMOTE_DEV_TERMINATION_DUE_TO_LOW_RESOURCES");
      break;
    case 21:
      Serial.println("REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF");
      break;
    case22:
      Serial.println("LOCAL_HOST_TERMINATED_CONNECTION");
      break;
    case 59:
      Serial.println("CONN_INTERVAL_UNACCEPTABLE");
      break;
    default: 
      Serial.print("Unknown disconnection reason. Code: ");
      Serial.println("params->reason");
      // if nothing else matches, do the default
      // default is optional
    break;
  }
  
  characteristic_is_found = 0;
  // start discovering BLE devices
  ble.startScan(scanCallBack);
}

/** @brief Discovered service callback handle
 *
 *  @param[in] *service  service->getUUID()  The UUID of service
 *                       service->getStartHandle()
 *                       service->getEndHandle()
 */
static void discoveredServiceCallBack(const DiscoveredService *service) {
  Serial.print("\n - ");
  if(service->getUUID().shortOrLong() == UUID::UUID_TYPE_SHORT) {
    Serial.print("Short (16bit) Service UUID: ");
    Serial.println(service->getUUID().getShortUUID(), HEX);
  } else {
    Serial.print("Long (128bit) Service UUID: ");
    uint8_t index;
    const uint8_t *uuid = service->getUUID().getBaseUUID();
    for(index=0; index<16; index++) {
      Serial.print(uuid[index], HEX);
      Serial.print(" ");
    }
  }
  Serial.print("\n   - Service handles: ");
  Serial.print(service->getStartHandle(), HEX);
  Serial.print(" to ");
  Serial.print(service->getEndHandle(), HEX);
}


/** @brief Discovered characteristics callback handle
 *
 *  @param[in] *chars  chars->getUUID()        The UUID of service
 *                     chars->getProperties()  broadcast() : Check if broadcasting is permitted. True is permited.
 *                                             read() : Check reading is permitted.
 *                                             writeWoResp() : Check if writing with Write Command is permitted
 *                                             write() : Check if writing with Write Request is permitted.
 *                                             notify() : Check notifications are permitted.
 *                                             indicate() : Check if indications are permitted.
 *                                             authSignedWrite() : Check if writing with Signed Write Command is permitted.
 *                     chars->getDeclHandle()  characteristic's declaration attribute handle
 *                     chars->getValueHandle() characteristic's value attribute handle
 *                     chars->getLastHandle()  characteristic's last attribute handle
 */
static void discoveredCharacteristicCallBack(const DiscoveredCharacteristic *chars) {
  Serial.println("\n    - Characteristics\n");
  Serial.print("Chars UUID type        : ");
  Serial.println(chars->getUUID().shortOrLong(), HEX);// 0 16bit_uuid, 1 128bit_uuid
  Serial.print("Chars UUID             : ");
  if(chars->getUUID().shortOrLong() == UUID::UUID_TYPE_SHORT) {
    Serial.println(chars->getUUID().getShortUUID(), HEX);
  }
  else {
    uint8_t index;
    const uint8_t *uuid = chars->getUUID().getBaseUUID();
    uint8_t match = 0;

    for(index=0; index<16; index++) {
      Serial.print(uuid[index], HEX);
      Serial.print(" ");
    }

    if (chars->getValueHandle() == 17){
      chars_led = *chars;
      Serial.print(" <-- Characteristic Found");
      characteristic_is_found = 1;
     }

    Serial.println(" ");
  }

  Serial.print("properties_read        : ");
  Serial.println(chars->getProperties().read(), DEC);
  Serial.print("properties_writeWoResp : ");
  Serial.println(chars->getProperties().writeWoResp(), DEC);
  Serial.print("properties_write       : ");
  Serial.println(chars->getProperties().write(), DEC);
  Serial.print("properties_notify      : ");
  Serial.println(chars->getProperties().notify(), DEC);

  Serial.print("declHandle             : ");
  Serial.println(chars->getDeclHandle(), HEX);
  Serial.print("valueHandle            : ");
  Serial.println(chars->getValueHandle(), HEX);
  Serial.print("lastHandle             : ");
  Serial.println(chars->getLastHandle(), HEX);
                           
}

/**
 * @brief Discovered service and characteristics termination callback handle
 */
static void discoveryTerminationCallBack(Gap::Handle_t connectionHandle) {
  Serial.print("\n4. Activity\n");
  Serial.print("connectionHandle       : ");
  Serial.println(connectionHandle, HEX);

  valueArray[1] = 0xEE; //Turn the light off

  // start the write - first writing with an led off command
  ble.gattClient().write(GattClient::GATT_OP_WRITE_REQ, chars_led.getConnectionHandle(), 0x11,    6,    (uint8_t *)&valueArray);       // data
}

/** @brief  write callback handle
 *
 *  @param[in] *params   params->connHandle : The handle of the connection that triggered the event
 *                       params->handle : Attribute Handle to which the write operation applies
 *                       params->writeOp : OP_INVALID               = 0x00,  // Invalid operation.
 *                                           OP_WRITE_REQ             = 0x01,  // Write request.
 *                                           OP_WRITE_CMD             = 0x02,  // Write command.
 *                                           OP_SIGN_WRITE_CMD        = 0x03,  // Signed write command.
 *                                           OP_PREP_WRITE_REQ        = 0x04,  // Prepare write request.
 *                                           OP_EXEC_WRITE_REQ_CANCEL = 0x05,  // Execute write request: cancel all prepared writes.
 *                                           OP_EXEC_WRITE_REQ_NOW    = 0x06,  // Execute write request: immediately execute all prepared writes.
 *                       params->offset : Offset for the write operation
 *                       params->len : Length (in bytes) of the data to write
 *                       params->data : Pointer to the data to write
 */

void onDataWriteCallBack(const GattWriteCallbackParams *params) {
//  Serial.println("GattClient write call back ");
//  Serial.print("The writeOp : ");
//  Serial.println(params->writeOp, HEX);
//  Serial.print("The handle : ");
//  Serial.println(params->handle, HEX);
//  Serial.print("The offset : ");
//  Serial.println(params->offset, DEC);
//  Serial.print("The len : ");
//  Serial.println(params->len, DEC);
//  Serial.print("The data : ");
//  for(uint8_t index=0; index<params->len; index++) {
//    Serial.print( params->data[index], HEX);
//  }

switch (lightMode) {
    case 0:
      valueArray[1] = 0xEE; // Turn the light off
      valueArray[2] = 0x00; 
      valueArray[3] = 0x00; 
      valueArray[4] = 0x00; 
      break;
    case 1:
      valueArray[1] = 0xAA; // Turn the light on to cool white mode
      valueArray[2] = 0x01; 
      valueArray[3] = brightness; //note: 99 is max
      valueArray[4] = 0x00; 
      break;
    case 2:
      valueArray[1] = 0xAA; // Turn the light on to warm white mode
      valueArray[2] = 0x01; 
      valueArray[3] = 0x00; 
      valueArray[4] = brightness; //note: 99 is max 
      break;
    case 3:
      valueArray[1] = 0xA1; // Turn the light on to colour mode
      valueArray[2] = brightness; // Red, 255 is max
      valueArray[3] = 0x00; 
      valueArray[4] = 0x00; 
      break;
    case 4:
      valueArray[1] = 0xA1; // Turn the light on to colour mode
      valueArray[2] = 0x00; 
      valueArray[3] = brightness; // Blue, 255 is max
      valueArray[4] = 0x00; 
      break;
    case 5:
      valueArray[1] = 0xA1; // Turn the light on to colour mode
      valueArray[2] = 0x00; 
      valueArray[3] = 0x00; 
      valueArray[4] = brightness; // Green, 255 is max
      break;
    case 6: // colour hue cycling
      if (colourAngle>=0 && colourAngle<60){
        redLed = 255;
        greenLed = 255*(colourAngle)/60;
        blueLed = 0;
        }
      else if (colourAngle>= 60 && colourAngle<120){
        redLed = 255*(120-colourAngle)/60;
        greenLed = 255;
        blueLed = 0;
        }
      else if (colourAngle>= 120 && colourAngle<180){
        redLed = 0;
        greenLed = 255;
        blueLed = 255*(colourAngle-120)/60;        
        }
      else if (colourAngle>= 180 && colourAngle<240){
        redLed = 0;
        greenLed = 255*(240-colourAngle)/60;
        blueLed = 255;
        }
       else if (colourAngle>= 240 && colourAngle<300){
        redLed = 255*(colourAngle-240)/60;
        greenLed = 0;
        blueLed = 255;
        }
      else if (colourAngle>= 300 && colourAngle<360){
        redLed = 255;
        greenLed = 0;
        blueLed = 255*(360-colourAngle)/60;
        }
      valueArray[1] = 0xA1; // Turn the light on to colour mode
      valueArray[2] = redLed; valueArray[3] = blueLed; valueArray[4] = greenLed;
      
      if (colourAngle < 360){lightMode--;colourAngle=colourAngle+3;} else {colourAngle = 0;}
      break;
    default: 
      // if nothing else matches, do the default
      // default is optional
    break;
  }

  // write data to light
  ble.gattClient().write(GattClient::GATT_OP_WRITE_REQ,chars_led.getConnectionHandle(),0x11,6,(uint8_t *)&valueArray);       // data
  
  lightMode++; if (lightMode == 7){lightMode=0;}
  if (colourAngle == 0){delay(1000);}//quick and dirty wait command.
}

void onDataReadCallBack(const GattReadCallbackParams *params) {
  Serial.println("GattClient read call back ");
  Serial.print("The handle : ");
  Serial.println(params->handle, HEX);
  Serial.print("The offset : ");
  Serial.println(params->offset, DEC);
  Serial.print("The len : ");
  Serial.println(params->len, DEC);
  Serial.print("The data : ");
  for(uint8_t index=0; index<params->len; index++) {
    if (params->data[index] == 0){Serial.print("00");} else {Serial.print( params->data[index], HEX);}
  }
  Serial.println("");
  // example of how to read data
  // ble.gattClient().read(chars_led.getConnectionHandle(), 0xE, 0);       // data
}

void setup() {
// open serial port
 Serial.begin(9600);
 Serial.print("\n0. Running...\n");
 ble.init(); // initiallise bluetooth
 ble.onConnection(connectionCallBack); //
 ble.onDisconnection(disconnectionCallBack);
 ble.gattClient().onServiceDiscoveryTermination(discoveryTerminationCallBack);
 ble.gattClient().onDataWrite(onDataWriteCallBack);
 ble.gattClient().onDataRead(onDataReadCallBack);
 ble.setScanParams(3000, 100, 0, true); // scan interval & scan window: ms between 2.5ms and 10.24s, timeout (s) between 0x0001 and 0xFFFF, 0x0000 disables timeout, activeScanning T/F 
 ble.startScan(scanCallBack);
 Serial.print("1. Starting Scan...\n");
}

void loop() {
  ble.waitForEvent(); // continuously respond to bluetooth events
}
