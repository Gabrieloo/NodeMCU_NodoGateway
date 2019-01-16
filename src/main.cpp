#include <Arduino.h>

//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 2. prints anything it receives to Serial.print
//
//
//************************************************************
#include "painlessMesh.h"

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

String sDatosSensorHumedadConcatenados;

struct Nodo{
	uint32_t nID;
	String sDatoSensorHumedad;
	bool bDatoActualObtenido;
} nodos[3];

bool bEsperarProximoCiclo;

// User stub
void informarNodoGateway() ; // Prototype so PlatformIO doesn't complain
void enviarTodosLosNodosADormir();
void EsperarProximoCiclo();

Task taskInformarNodoGateway(TASK_SECOND * 1, TASK_FOREVER, &informarNodoGateway, &userScheduler, true);
Task taskEnviarComandoTodosLosNodosADormir(TASK_SECOND * 1, TASK_FOREVER, &enviarTodosLosNodosADormir, &userScheduler, false);
Task taskEsperarProximoCiclo(TASK_MINUTE * 1, TASK_ONCE, &EsperarProximoCiclo, &userScheduler, false);

void informarNodoGateway() {

  String msg = "soyNodoGateway";
  mesh.sendBroadcast( msg );

}

void enviarTodosLosNodosADormir(){

	String comandoADormir = "TodosLosNodosADormir";
	mesh.sendBroadcast(comandoADormir);
	Serial.printf("TodosLosNodosADormir");

}

void EsperarProximoCiclo(){
	bEsperarProximoCiclo = false;
	taskEnviarComandoTodosLosNodosADormir.disable();

	//String msg = "soyNodoGateway";
  //mesh.sendBroadcast( msg );//

}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {


	Serial.printf("MENSAJE: from %u msg=%s\n", from, msg.c_str());

	String recibido = "recibido";
	mesh.sendSingle(from, recibido);

  //Serial.printf("%u/%s\n", from, msg.c_str()); // debug

	//Serial.printf("%u/%s\n", from, msg.substring(0,5).c_str());	// debug
	//Serial.printf("%u/%s\n", from, msg.substring(5).c_str());	// debug

/*/////
	for(int k=0; k<=1 ; k++){
		sDatosSensorHumedadConcatenados += nodos[k].sDatoSensorHumedad + ";";
	}
	Serial.printf("%s\n", sDatosSensorHumedadConcatenados.c_str());
	///////*/
	if(msg.substring(0,5) == "Dato_" && bEsperarProximoCiclo == false){

		String sDatoSensorHumedadSensor = msg.substring(5);

		for(int i=0; i<=2 ; i++){

	    //Serial.printf("1er for %u/%s\n", from, msg.c_str());// debug

	    if(from == nodos[i].nID){

	      //Serial.printf("2do for %u/%s\n", from, msg.c_str());// debug

	      nodos[i].sDatoSensorHumedad = sDatoSensorHumedadSensor;

				if(nodos[i].bDatoActualObtenido == false){
						Serial.printf("Dato_%u/%s\n", from, msg.substring(5).c_str()); //Serial a Node-Red
						nodos[i].bDatoActualObtenido = true;
				}else{
						break;
				}



	      for(int j=0; j<=2 ; j++){

					//Serial.printf("3er for %u/%s\n", from, msg.c_str());// debug
	        if(nodos[j].bDatoActualObtenido == false){
	          break;
	        }

	        if(j == 2){//todos los datos fueron recibidos

						//Serial.printf("todos obtenidos (j==2) %u/%s\n", from, msg.c_str());// debug

						/*for(int k=0; k<=2 ; k++){	// Par enviar todos los datos concatenados una vez recibidos de todos los nodos

							Serial.printf("4to for %u/%s\n", from, msg.c_str());// debug
							sDatosSensorHumedadConcatenados += nodos[k].sDatoSensorHumedad + ";";
						}

						//Enviar array de datos por serial, tomarlos en Node-RED por relación PosiciónArray-Nodo preestablecido
						Serial.printf("%s\n", sDatosSensorHumedadConcatenados.c_str());
						*/

						//Reset de estado de datos recibidos
						nodos[0].bDatoActualObtenido = false;
						nodos[1].bDatoActualObtenido = false;
						nodos[2].bDatoActualObtenido = false;

						sDatosSensorHumedadConcatenados = "";

						//Mandar a dormir a todos los nodos de la red
						taskEnviarComandoTodosLosNodosADormir.enable();

						//Esperar al próximo ciclo para recibir datos (15 min default, 1 min para test)
						bEsperarProximoCiclo = true;
						taskEsperarProximoCiclo.restartDelayed();
	        }

	      }

	    }

	  }
	}



}

void newConnectionCallback(uint32_t nodeId) {
    //Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
    //Serial.printf("Changed connections %s\n",mesh.subConnectionJson().c_str());
}

void nodeTimeAdjustedCallback(int32_t offset) {
    //Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {

  nodos[0] = {2487867698,"-1",false};
  nodos[1] = {2133245594,"-1",false};
  nodos[2] = {2485386851,"-1",false};

	sDatosSensorHumedadConcatenados = "";
	bEsperarProximoCiclo = false;

  Serial.begin(115200);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskInformarNodoGateway );
  taskInformarNodoGateway.enable();
}

void loop() {
  userScheduler.execute(); // it will run mesh scheduler as well
  mesh.update();
}

// v0.0.3 2018/01/16 taskEsperarProximoCiclo
