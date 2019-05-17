function [ points,triangles,move_point_idx ] = splitTriangles( points,triangles,idx_thr)
%SPLITTRIANGLES Summary of this function goes here
%   Detailed explanation goes here


mid_points=1.0/3.0*(points(:,triangles(1,idx_thr))+points(:,triangles(2,idx_thr))+points(:,triangles(3,idx_thr)));

move_point_idx=[1:size(mid_points,2)]+size(points,2);

points=[points mid_points];


triangles(:,idx_thr)=[];
C=[triangles(1,:) triangles(2,:) triangles(3,:);triangles(2,:) triangles(3,:) triangles(1,:) ];
%triangles=delaunay(points(1,:),points(2,:),C)';
DT=delaunayTriangulation(points(1,:)',points(2,:)');
triangles=DT.ConnectivityList';
points=DT.Points';

[OrderedNeighborsCell]=GetNeighbors(points,triangles);
tmp_neigh=OrderedNeighborsCell(move_point_idx);
tmp_neigh = cell2mat(tmp_neigh(:));
move_point_idx=unique([ move_point_idx tmp_neigh(:)']);
end

