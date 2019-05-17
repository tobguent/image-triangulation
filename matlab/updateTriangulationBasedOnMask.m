function [ points,triangles ] = updateTriangulationBasedOnMask( mask,number_of_points,points,triangles)
%GETFEATUREGRID Summary of this function goes here
%   Detailed explanation goes here
    
    logicalMask=mask>0;
    points_round=round(points);
    
    [dimY,dimX]=size(mask);
    
    idx = sub2ind([dimY, dimX],points_round(2,:), points_round(1,:));
    poi_idx=find(logicalMask(idx));

    all_col=[];
    for i=1:size(poi_idx,2)
        [row_idx,col_idx]=find(triangles==poi_idx(i));
        all_col=[all_col col_idx'];
    end
    all_col=unique(all_col);
    
    triangles(:,all_col)=[];
    
    current_sub=0;
    for i=1:size(poi_idx,2)
        idx=find(triangles>poi_idx(i)-current_sub);
        triangles(idx)=triangles(idx)-1;
        points_round(:,poi_idx(i)-current_sub)=[];
        current_sub=current_sub+1;
        
    end
    
    C=[triangles(1,:) triangles(2,:) triangles(3,:);triangles(2,:) triangles(3,:) triangles(1,:) ];

    mask_rs=reshape(mask,1,[]);
    
    x=1:length(mask_rs);

    xq=1:0.05:length(mask_rs);
    pdf_int=interp1(x,mask_rs,xq,'spline');
    pdf_int(pdf_int<0)=0;
    pdf_int=pdf_int/sum(pdf_int);

    cdf_int=cumsum(pdf_int);

    [cdf_int,mask]=unique(cdf_int);
    xq=xq(mask);

    randomValues=rand(1,number_of_points);

    projection=interp1(cdf_int,xq,randomValues);

    x=floor((projection-1)/dimY)+1;
    y=round(projection-(dimY)*(x-1));
    
	points=[points_round(1,:) x;points_round(2,:) y];
      
	DT=delaunayTriangulation(points(1,:)',points(2,:)',C');
	triangles=DT.ConnectivityList';
	points=DT.Points';

end

