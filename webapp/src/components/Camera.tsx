import React from "react";

function Camera() {
  const [imgSrcList, setImgSrcList] = React.useState(["empty"]);

  React.useEffect(() => {
    async function getImgSrcList() {
      const imgSrcResult = await fetch(
        "https://608dev-2.net/sandbox/sc/team10/server.py?allcamera=1"
      );
      if (imgSrcResult) {
        const jsonResult = await imgSrcResult.json();
        const imgList = jsonResult["result"];
        setImgSrcList(imgList);
        console.log(imgList);
      }
    }
    getImgSrcList();
  }, [imgSrcList]);

  const camera = (
    <div className="relative ">
      <div className="text-xl mb-5 mt-10 flex flex-row justify-center font-serif font-semibold text-cyan-800">
        Live Feed from the camera!
      </div>
      <hr className="w-2/4 ml-auto mr-auto mb-16 text-black border-2 border-slate-800" />
      <div className="relative flex flex-row justify-center ">
        <div className="bg-white h-50 rounded-lg justify-center border-8 border-cyan-500">
          <img
            src={imgSrcList[0][0]}
            className="object-contain h-full w-full"
            alt={"camera output"}
            width={"200"}
          />
        </div>
      </div>
      <hr className="w-3/4 ml-auto mr-auto mt-20 mb-1 text-black border-2 border-slate-800" />
      <hr className="w-3/4 ml-auto mr-auto mb-16 text-black border-2 border-slate-800" />
      <div className="text-xl mb-5 mt-10 flex flex-row justify-center font-serif font-semibold text-pink-600">
        Previous images and locations
      </div>
      <hr className="w-2/4 ml-auto mr-auto mb-16 text-black border-2 border-slate-800" />
      <div className="relative flex flex-col justify-center max-w-40">
        {imgSrcList.map((img, index) => {
          const date = new Date(img[3][0]);
          const dateStr = date.toUTCString();
          return (
            <div>
              <div className="w-fit bg-white h-50 rounded-lg justify-center border-8 border-pink-300 ml-auto mr-auto mx-auto">
                <img
                  src={img[0]}
                  className="object-contain"
                  alt={"camera output"}
                  width="200"
                />
              </div>
              <div className="flex flex-col mt-3 mb-16 p-5 items-center font-semibold font-mono">
                <div>{img[1]}</div>
                <div>
                  <div>Time: {dateStr}</div>
                  <div>Latitude: {img[3][1]}</div>
                  <div>Longitude: {img[3][2]}</div>
                  <div>Location: {img[3][3]}</div>
                </div>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );

  return <div className="flex flex-col h-fit bg-pink-100">{camera}</div>;
}

export default Camera;
