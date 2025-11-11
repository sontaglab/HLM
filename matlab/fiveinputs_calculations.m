clear
N = [5,6,7,8,9,10,11,12,13,14,15,16];
cost = [4,17,50,139,306,619,965,1237,1004,601,230,90];
%cost = cumsum(cost);
fraction = cumsum(cost)/sum(cost);
%plot(N,cumsum(cost)/sum(cost))

for l = 7:13
    counter = 0;

    for i = 1:10
        load(sprintf('PI_5inputs_rand%i.mat',i))
        id = zeros(10000,1);
        for j = 1:10000
           for k = 1:length(nb_norgates{j,1})
               if nb_norgates{j,1}(k) > l
                   id(j) = id(j) + 1;
               end
           end
        end
        counter = counter + sum(id < 5);
    end

    counter = counter /1000
end