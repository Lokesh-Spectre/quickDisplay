import mqtt from "mqtt";
import { server } from "./constants.js";

const client = mqtt.connect(server.mqtt_broker_url) 

await new Promise((resolve)=>{
    client.on('connect',()=>{
        console.log("connected with MQTT broker...");
        client.subscribe("/cosmicForge/Welcome",()=>{console.log("Subscribed to welcome")});
        resolve();
    })
})
export {client};
export default client;