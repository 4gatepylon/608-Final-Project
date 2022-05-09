import React from "react";

function Camera() {
  const [imgSrcList, setImgSrcList] = React.useState(["empty"]);

  React.useEffect(() => {
    async function getImgSrcList() {
      const imgSrcResult = await fetch(
        "https://608dev-2.net/sandbox/sc/team10/server.py?camera=1"
      );
      if (imgSrcResult) {
        const jsonResult = await imgSrcResult.json();
        const imgSrcString: string = jsonResult["image_decoded"];
        const newImgSrcString = [imgSrcString, imgSrcString, imgSrcString];
        setImgSrcList(newImgSrcString);
      }

      //setImgSrc(imgSrcResult);
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
            src={imgSrcList[0]}
            className="object-contain h-full w-full"
            alt={"camera output"}
          />
        </div>
      </div>
      {/* <hr className="w-3/4 ml-auto mr-auto mt-20 mb-1 text-black border-2 border-slate-800" />
      <hr className="w-3/4 ml-auto mr-auto mb-16 text-black border-2 border-slate-800" />
      <div className="text-xl mb-5 mt-10 flex flex-row justify-center font-serif font-semibold text-pink-600">
        Previous images and locations
      </div>
      <hr className="w-2/4 ml-auto mr-auto mb-16 text-black border-2 border-slate-800" />
      <div className="relative flex flex-col justify-center max-w-40">
        {imgSrcList.map((imgSrc, index) => {
          return (
            <div className="bg-white h-50 rounded-lg justify-center border-8 border-pink-300 ml-auto mr-auto mb-16">
              <img
                src={imgSrc}
                className="object-contain"
                alt={"camera output"}
              />
            </div>
          );
        })}
      </div> */}
    </div>
  );

  return <div className="flex flex-col h-screen bg-pink-100">{camera}</div>;
}

export default Camera;
