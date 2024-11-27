import {client} from "./mqtt.js";
import welcome from "./mqtt_services/welcome.js";
export default function(){
    client.subscribe("/#/Welcome");

    client.on("message",(topic,payload)=>{
        
        switch(topic.split("/").at(-1)){
        
            case "Welcome":
                welcome(topic,payload);
                break;
        
        }
        console.log(`topic: ${topic}\npayload:${payload}`);
    })
}
