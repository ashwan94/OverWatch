
// MQTT client
var mqttClient= null;

//MQTT info
// 각자 상황에 맞는 host, port, topic 을 사용합니다.
const mqtt_host = "nextit.or.kr";
const mqtt_port = "29001";
const mqtt_clientId = "OverWatch" + parseInt(Math.random() * 100);        // 랜덤 클라이언트 ID
const mqtt_topic = "/IoT/Sensor/2ndClass/OverWatch";

// MQTT 클라이언트 연결
function StartMqtt()
{
    mqttClient = new Paho.MQTT.Client(mqtt_host, Number(mqtt_port), mqtt_clientId);

    mqttClient.onConnectionLost = onConnectionLost;
    mqttClient.onMessageArrived = onMessageArrived;

    mqttClient.connect({
        onSuccess : onConnect
        , onFailure : onFailure
    });
}

// 연결 성공 시 실행되는 function
function onConnect()
{
    console.log("Connect to " + mqtt_host + " is OK!!");
    console.log("subscribe to " + mqtt_topic);
    mqttClient.subscribe(mqtt_topic);
}

// 연결 실패 시 실행되는 function
function onFailure()
{
    console.log("connet : onFailure..");
}



function onConnectionLost(responseObject)
{
    if (responseObject.errorCode !== 0)
    {
        console.log("onConnectionLost : " + responseObject.errorMessage);

        // 연결 재시도
        StartMqtt();
    }
}

// 메시지 받는 부분
function onMessageArrived(message)
{
    console.log(mqtt_clientId + " is onMessageArrived");
    const received_data = JSON.parse(message.payloadString); // JSON Parsing
    // console.log("받은 data : ", received_data);

    // HTML 의 function 으로 parameter 넘기기
    addData(received_data);
}



// 메시지 보내기
// 각 화면에서 메시지를 보내려면 각 화면에서 아래 function 선언하여 사용
function fncMqttDoSend(data)
{
    mqttClient.send(mqtt_topic, data);
}
