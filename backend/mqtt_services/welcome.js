import { publishTime } from "../jobs/timeJob.js";
async function initialTimePublish() {
    // await new Promise(resolve=>setTimeout(resolve,100));
    publishTime();
}
export default function (topic,payload){
    if(!topic.endsWith("Welcome")){
        return;
    }
    console.log(`Device connected with MAC address: ${payload}`);
    initialTimePublish();
}
