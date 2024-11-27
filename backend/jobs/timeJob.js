import client from "../mqtt.js";

function publishTime(){
    const time = Math.floor(Date.now()/1000);
    client.publish("/cosmicForge/time",time.toString());
    console.log("Published Time:", + time.toString());
}
function timeJob (){
    publishTime();
    return setInterval(
        publishTime
        ,10*60*1000);
}
export {publishTime, timeJob};
export default timeJob;
    