function [ points,triangles,neighborList,neighborStartIndex,numberOfNeighbors] = getRegularGrid( dimX,dimY,numberOfPoints_X,numberOfPoints_Y)


%%%%%%%construct triangulation%%%%%%%%%
numberOfPoints=numberOfPoints_X*numberOfPoints_Y;

points=zeros(2,numberOfPoints);

tmp=round((0:numberOfPoints_X-1).*((dimX-1)/(numberOfPoints_X-1))+1);
points(1,:)=repmat(tmp,1,numberOfPoints_Y);


tmp=round((0:numberOfPoints_Y-1).*((dimY-1)/(numberOfPoints_Y-1))+1);
tmp=repmat(tmp,1,numberOfPoints_X);

tmp=reshape(tmp,numberOfPoints_Y,numberOfPoints_X);
points(2,:)=reshape(tmp',1,[]);


numberTri=2*(numberOfPoints_X-1)*(numberOfPoints_Y-1);
triangles=zeros(3,numberTri);



tmp=zeros(1,2*(numberOfPoints_X-1));
tmp(1:2:end-1)=1:numberOfPoints_X-1;
tmp(2:2:end)=2:numberOfPoints_X;
tmp2=(1:numberTri);
tmp2=(tmp2-mod(tmp2-1,2*(numberOfPoints_X-1))-1)/(2*(numberOfPoints_X-1));
triangles(1,:)=repmat(tmp,1,numberOfPoints_Y-1)+tmp2*numberOfPoints_X;


triangles(3,1:2:end)=triangles(1,1:2:end)+numberOfPoints_X;
triangles(3,2:2:end)=triangles(1,2:2:end)+numberOfPoints_X-1;

triangles(2,1:2:end)=triangles(1,1:2:end)+1;
triangles(2,2:2:end)=triangles(3,2:2:end)+1;


nv=size(points,2);
nt=size(triangles,2);

IncidenceMatrix=sparse([1:nt,1:nt,1:nt],[triangles(1,:),triangles(2,:),triangles(3,:)],1,nt,nv);

neighborList=[];
neighborStartIndex=zeros(1,nv);
numberOfNeighbors=zeros(1,nv);

for i=1:nv
    tri_n=find(IncidenceMatrix(:,i)==1)';
    neighborList=[neighborList tri_n];
    numberOfNeighbors(i)=length(tri_n);
    if(i==1)
        neighborStartIndex(i)=1;
    else
        neighborStartIndex(i)=neighborStartIndex(i-1)+numberOfNeighbors(i-1);
    end
end
%         hold on;
%         scatter(points(1,:),points(2,:));
%         triplot(triangles',points(1,:),points(2,:));
%         hold off;
end

